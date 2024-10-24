#include "pch.h"
#include "AssimpAnimationImporter.h"

#include "Engine/Core/Log.h"
#include "Engine/Utilities/AssimpLogStream.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#include <set>
#include <unordered_map>
#include <unordered_set>

namespace Engine {

	namespace Utils {
		glm::mat4 Mat4FromAIMatrix4x4(const aiMatrix4x4& matrix);

		float AngleAroundYAxis(const glm::quat& quat)
		{
			static glm::vec3 xAxis = { 1.0f, 0.0f, 0.0f };
			static glm::vec3 yAxis = { 0.0f, 1.0f, 0.0f };
			auto rotatedOrthogonal = quat * xAxis;
			auto projected = glm::normalize(rotatedOrthogonal - (yAxis * glm::dot(rotatedOrthogonal, yAxis)));
			return acos(glm::dot(xAxis, projected));
		}
	}


	namespace AssimpAnimationImporter {
		static const uint32_t s_AnimationImportFlags =
			aiProcess_CalcTangentSpace |        // Create binormals/tangents just in case
			aiProcess_Triangulate |             // Make sure we're triangles
			aiProcess_SortByPType |             // Split meshes by primitive type
			aiProcess_GenNormals |              // Make sure we have legit normals
			aiProcess_GenUVCoords |             // Convert UVs if required
//			aiProcess_OptimizeGraph |
			aiProcess_OptimizeMeshes |          // Batch draws where possible
			aiProcess_JoinIdenticalVertices |
			aiProcess_LimitBoneWeights |        // If more than N (=4) bone weights, discard least influencing bones and renormalise sum to 1
			aiProcess_GlobalScale |             // e.g. convert cm to m for fbx import (and other formats where cm is native)
//			aiProcess_PopulateArmatureData |    // not currently using this data
			aiProcess_ValidateDataStructure;    // Validation 


		class BoneHierarchy
		{
		public:
			BoneHierarchy(const aiScene* scene);

			void ExtractBones();
			void TraverseNode(aiNode* node, Skeleton* skeleton);
			void TraverseBone(aiNode* node, Skeleton* skeleton, uint32_t parentIndex);
			Scope<Skeleton> CreateSkeleton();

		private:
			std::set<std::string_view> m_Bones;
			const aiScene* m_Scene;
		};


		Scope<Skeleton> ImportSkeleton(const std::string_view filename)
		{
			AssimpLogStream::Initialize();

			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile(filename.data(), s_AnimationImportFlags);
			return ImportSkeleton(scene);
		}


		Scope<Skeleton> ImportSkeleton(const aiScene* scene)
		{
			BoneHierarchy boneHierarchy(scene);
			return boneHierarchy.CreateSkeleton();
		}


		std::vector<std::string> GetAnimationNames(const aiScene* scene)
		{
			std::vector<std::string> animationNames;
			if (scene)
			{
				animationNames.reserve(scene->mNumAnimations);
				for (size_t i = 0; i < scene->mNumAnimations; ++i)
				{
					if (scene->mAnimations[i]->mDuration > 0.0f)
					{
						animationNames.emplace_back(scene->mAnimations[i]->mName.C_Str());
					} else
					{
						ZONG_CONSOLE_LOG_WARN("Animation '{0}' duration is zero or negative.  This animation was ignored!", scene->mAnimations[i]->mName.C_Str());
					}
				}
			}
			return animationNames;
		}


		template<typename T> struct KeyFrame
		{
			float FrameTime;
			T Value;
			KeyFrame(const float frameTime, const T& value) : FrameTime(frameTime), Value(value) {}
		};


		struct Channel
		{
			std::vector<KeyFrame<glm::vec3>> Translations;
			std::vector<KeyFrame<glm::quat>> Rotations;
			std::vector<KeyFrame<glm::vec3>> Scales;
			uint32_t Index;
		};


		// Import all of the channels from anim that refer to bones in skeleton
		static auto ImportChannels(const aiAnimation* anim, const Skeleton& skeleton, const bool isMaskedRootMotion, const glm::vec3& rootTranslationMask, float rootRotationMask)
		{
			std::vector<Channel> channels;

			std::unordered_map<std::string_view, uint32_t> boneIndices;
			std::unordered_set<uint32_t> rootBoneIndices;
			for (uint32_t i = 0; i < skeleton.GetNumBones(); ++i)
			{
				boneIndices.emplace(skeleton.GetBoneName(i), i + 1);  // 0 is reserved for root motion channel    boneIndices are base=1
				if (skeleton.GetParentBoneIndex(i) == Skeleton::NullIndex)
					rootBoneIndices.emplace(i+1);
			}

			std::map<uint32_t, aiNodeAnim*> validChannels;
			for (uint32_t channelIndex = 0; channelIndex < anim->mNumChannels; ++channelIndex)
			{
				aiNodeAnim* nodeAnim = anim->mChannels[channelIndex];
				auto it = boneIndices.find(nodeAnim->mNodeName.C_Str());
				if (it != boneIndices.end())
				{
					validChannels.emplace(it->second, nodeAnim);   // validChannels.first is base=1,  .second is node pointer
				}
			}

			channels.resize(skeleton.GetNumBones() + 1);   // channels is base=1
			for (uint32_t boneIndex = 1; boneIndex < channels.size(); ++boneIndex)
			{
				Channel& channel = channels[boneIndex];
				channel.Index = boneIndex;
				if (auto validChannel = validChannels.find(boneIndex); validChannel != validChannels.end())
				{
					auto nodeAnim = validChannel->second;
					channel.Translations.reserve(nodeAnim->mNumPositionKeys + 2); // +2 because worst case we insert two more keys
					channel.Rotations.reserve(nodeAnim->mNumRotationKeys + 2);
					channel.Scales.reserve(nodeAnim->mNumScalingKeys + 2);

					// Note: There is no need to check for duplicate keys (i.e. multiple keys all at same frame time)
					//       because Assimp throws these out for us
					for (uint32_t keyIndex = 0; keyIndex < nodeAnim->mNumPositionKeys; ++keyIndex)
					{
						aiVectorKey key = nodeAnim->mPositionKeys[keyIndex];
						float frameTime = std::clamp(static_cast<float>(key.mTime / anim->mDuration), 0.0f, 1.0f);
						if ((keyIndex == 0) && (frameTime > 0.0f))
						{
							channels[boneIndex].Translations.emplace_back(0.0f, glm::vec3{ static_cast<float>(key.mValue.x), static_cast<float>(key.mValue.y), static_cast<float>(key.mValue.z) });
						}
						channel.Translations.emplace_back(frameTime, glm::vec3{ static_cast<float>(key.mValue.x), static_cast<float>(key.mValue.y), static_cast<float>(key.mValue.z) });
					}
					if (channel.Translations.empty())
					{
						ZONG_CORE_WARN_TAG("Animation", "No translation track found for bone '{}'", skeleton.GetBoneName(boneIndex - 1));
						channel.Translations = { {0.0f, glm::vec3{0.0f}}, {1.0f, glm::vec3{0.0f}} };
					}
					else if (channel.Translations.back().FrameTime < 1.0f)
					{
						channel.Translations.emplace_back(1.0f, channel.Translations.back().Value);
					}
					for (uint32_t keyIndex = 0; keyIndex < nodeAnim->mNumRotationKeys; ++keyIndex)
					{
						aiQuatKey key = nodeAnim->mRotationKeys[keyIndex];
						float frameTime = std::clamp(static_cast<float>(key.mTime / anim->mDuration), 0.0f, 1.0f);

						// WARNING: constructor parameter order for a quat is still WXYZ even if you have defined GLM_FORCE_QUAT_DATA_XYZW
						if ((keyIndex == 0) && (frameTime > 0.0f))
						{
							channel.Rotations.emplace_back(0.0f, glm::quat{ static_cast<float>(key.mValue.w), static_cast<float>(key.mValue.x), static_cast<float>(key.mValue.y), static_cast<float>(key.mValue.z) });
						}
						channel.Rotations.emplace_back(frameTime, glm::quat{ static_cast<float>(key.mValue.w), static_cast<float>(key.mValue.x), static_cast<float>(key.mValue.y), static_cast<float>(key.mValue.z) });
						ZONG_CORE_ASSERT(fabs(glm::length(channels[boneIndex].Rotations.back().Value) - 1.0f) < 0.00001f);   // check rotations are normalized (I think assimp ensures this, but not 100% sure)
					}
					if (channel.Rotations.empty())
					{
						ZONG_CORE_WARN_TAG("Animation", "No rotation track found for bone '{}'", skeleton.GetBoneName(boneIndex - 1));
						channel.Rotations = { {0.0f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}}, {1.0f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}} };
					}
					else if (channel.Rotations.back().FrameTime < 1.0f)
					{
						channel.Rotations.emplace_back(1.0f, channel.Rotations.back().Value);
					}
					for (uint32_t keyIndex = 0; keyIndex < nodeAnim->mNumScalingKeys; ++keyIndex)
					{
						aiVectorKey key = nodeAnim->mScalingKeys[keyIndex];
						float frameTime = std::clamp(static_cast<float>(key.mTime / anim->mDuration), 0.0f, 1.0f);
						if (keyIndex == 0 && frameTime > 0.0f)
						{
							channel.Scales.emplace_back(0.0f, glm::vec3{ static_cast<float>(key.mValue.x), static_cast<float>(key.mValue.y), static_cast<float>(key.mValue.z) });
						}
						channel.Scales.emplace_back(frameTime, glm::vec3{ static_cast<float>(key.mValue.x), static_cast<float>(key.mValue.y), static_cast<float>(key.mValue.z) });
					}
					if (channel.Scales.empty())
					{
						ZONG_CORE_WARN_TAG("Animation", "No scale track found for bone '{}'", skeleton.GetBoneName(boneIndex - 1));
						channel.Scales = { {0.0f, glm::vec3{1.0f}}, {1.0f, glm::vec3{1.0f}} };
					}
					else if (channel.Scales.back().FrameTime < 1.0f)
					{
						channel.Scales.emplace_back(1.0f, channels[boneIndex].Scales.back().Value);
					}
				}
				else
				{
					ZONG_CORE_WARN_TAG("Animation", "No animation tracks found for bone '{}'", skeleton.GetBoneName(boneIndex - 1));
					channel.Translations = { {0.0f, glm::vec3{0.0f}}, {1.0f, glm::vec3{0.0f}} };
					channel.Rotations = { {0.0f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}}, {1.0f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}} };
					channel.Scales = { {0.0f, glm::vec3{1.0f}}, {1.0f, glm::vec3{1.0f}} }; 
				}
			}

			// Create root motion channel.
			// If isMaskedRootMotion is true, then root motion channel is created by filtering components of the first channel.
			// Otherwise root motion channel is copied as-is from the first channel.
			//
			// Root motion is then removed from all "root" channels (so it doesn't get applied twice)

			ZONG_CORE_ASSERT(!rootBoneIndices.empty()); // Can't see how this would ever be false!
			ZONG_CORE_ASSERT(rootBoneIndices.find(1) != rootBoneIndices.end()); // First bone must be a root!

			Channel& root = channels[0];
			root.Index = 0;
			if (isMaskedRootMotion)
			{
				for (auto& translation : channels[1].Translations)
				{
					root.Translations.emplace_back(translation.FrameTime, translation.Value * rootTranslationMask);
					translation.Value *= (glm::vec3(1.0f) - rootTranslationMask);
				}
				for (auto& rotation : channels[1].Rotations)
				{
					if (rootRotationMask > 0.0f)
					{
						auto angleY = Utils::AngleAroundYAxis(rotation.Value);
						root.Rotations.emplace_back(rotation.FrameTime, glm::quat{ glm::cos(angleY * 0.5f), glm::vec3{0.0f, 1.0f, 0.0f} * glm::sin(angleY * 0.5f) });
						rotation.Value = glm::conjugate(glm::quat(glm::cos(angleY * 0.5f), glm::vec3{ 0.0f, 1.0f, 0.0f } * glm::sin(angleY * 0.5f))) * rotation.Value;
					}
					else
					{
						root.Rotations.emplace_back(rotation.FrameTime, glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f });
					}
				}
			}
			else
			{
				root.Translations = channels[1].Translations;
				root.Rotations = channels[1].Rotations;
				channels[1].Translations = { {0.0f, glm::vec3{0.0f}}, {1.0f, glm::vec3{0.0f}} };
				channels[1].Rotations = { {0.0f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}}, {1.0f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}} };
			}
			root.Scales = { {0.0f, glm::vec3{1.0f}}, {1.0f, glm::vec3{1.0f}} };

			// It is possible that there is more than one "root" bone in the asset.
			// We need to remove the root motion from all of them (otherwise those bones will move twice as fast when root motion is applied)
			auto& rootMotion = channels[0];
			for (const auto rootBoneIndex : rootBoneIndices)
			{
				// we already removed root motion from the first bone, above
				if (rootBoneIndex != 1)
				{
					for (auto& translation : channels[rootBoneIndex].Translations)
					{
						// sample root motion channel at the this translation's frametime
						for(size_t rootMotionFrame = 0; rootMotionFrame < rootMotion.Translations.size() - 1; ++rootMotionFrame)
						{
							if (rootMotion.Translations[rootMotionFrame + 1].FrameTime >= translation.FrameTime)
							{
								const float alpha = (translation.FrameTime - rootMotion.Translations[rootMotionFrame].FrameTime) / (rootMotion.Translations[rootMotionFrame + 1].FrameTime - rootMotion.Translations[rootMotionFrame].FrameTime);
								translation.Value -= glm::mix(rootMotion.Translations[rootMotionFrame].Value, rootMotion.Translations[rootMotionFrame + 1].Value, alpha);
								break;
							}
						}
					}

					for (auto& rotation : channels[rootBoneIndex].Rotations)
					{
						// sample root motion channel at the this rotation's frametime
						for (size_t rootMotionFrame = 0; rootMotionFrame < rootMotion.Rotations.size() - 1; ++rootMotionFrame)
						{
							if (rootMotion.Rotations[rootMotionFrame + 1].FrameTime >= rotation.FrameTime)
							{
								const float alpha = (rotation.FrameTime - rootMotion.Rotations[rootMotionFrame].FrameTime) / (rootMotion.Rotations[rootMotionFrame + 1].FrameTime - rootMotion.Rotations[rootMotionFrame].FrameTime);
								rotation.Value = glm::normalize(glm::conjugate(glm::slerp(rootMotion.Rotations[rootMotionFrame].Value, rootMotion.Rotations[rootMotionFrame + 1].Value, alpha)) * rotation.Value);
								break;
							}
						}
					}
				}
			}
			return channels;
		}


		static auto ConcatenateChannelsAndSort(const std::vector<Channel>& channels)
		{
			// We concatenate the translations for all the channels into one big long vector, and then sort
			// it on _previous_ frame time.  This gives us an efficient way to sample the key frames later on.
			// (taking advantage of fact that animation almost always plays forwards)

			uint32_t numTranslations = 0;
			uint32_t numRotations = 0;
			uint32_t numScales = 0;

			for (auto channel : channels)
			{
				numTranslations += static_cast<uint32_t>(channel.Translations.size());
				numRotations += static_cast<uint32_t>(channel.Rotations.size());
				numScales += static_cast<uint32_t>(channel.Scales.size());
			}

			std::vector<std::pair<float, TranslationKey>> translationKeysTemp;
			std::vector<std::pair<float, RotationKey>> rotationKeysTemp;
			std::vector<std::pair<float, ScaleKey>> scaleKeysTemp;
			translationKeysTemp.reserve(numTranslations);
			rotationKeysTemp.reserve(numRotations);
			scaleKeysTemp.reserve(numScales);
			for (const auto& channel : channels)
			{
				float prevFrameTime = -1.0f;
				for (const auto& translation : channel.Translations)
				{
					translationKeysTemp.emplace_back(prevFrameTime, TranslationKey{ translation.FrameTime, channel.Index, translation.Value });
					prevFrameTime = translation.FrameTime;
				}

				prevFrameTime = -1.0f;
				for (const auto& rotation : channel.Rotations)
				{
					rotationKeysTemp.emplace_back(prevFrameTime, RotationKey{ rotation.FrameTime, channel.Index, rotation.Value });
					prevFrameTime = rotation.FrameTime;
				}

				prevFrameTime = -1.0f;
				for (const auto& scale : channel.Scales)
				{
					scaleKeysTemp.emplace_back(prevFrameTime, ScaleKey{ scale.FrameTime, channel.Index, scale.Value });
					prevFrameTime = scale.FrameTime;
				}
			}
			std::sort(translationKeysTemp.begin(), translationKeysTemp.end(), [](const auto& a, const auto& b) { return (a.first < b.first) || ((a.first == b.first) && a.second.Track < b.second.Track); });
			std::sort(rotationKeysTemp.begin(), rotationKeysTemp.end(), [](const auto& a, const auto& b) { return (a.first < b.first) || ((a.first == b.first) && a.second.Track < b.second.Track); });
			std::sort(scaleKeysTemp.begin(), scaleKeysTemp.end(), [](const auto& a, const auto& b) { return (a.first < b.first) || ((a.first == b.first) && a.second.Track < b.second.Track); });

			return std::tuple{ translationKeysTemp, rotationKeysTemp, scaleKeysTemp };
		}


		static auto ExtractKeys(const std::vector<std::pair<float, TranslationKey>>& translations, const std::vector<std::pair<float, RotationKey>>& rotations, const std::vector<std::pair<float, ScaleKey>>& scales)
		{
			std::vector<TranslationKey> translationKeyFrames;
			std::vector<RotationKey> rotationKeyFrames;
			std::vector<ScaleKey> scaleKeyFrames;
			translationKeyFrames.reserve(translations.size());
			rotationKeyFrames.reserve(rotations.size());
			scaleKeyFrames.reserve(scales.size());
			for (const auto& translation : translations)
			{
				translationKeyFrames.emplace_back(translation.second);
			}
			for (const auto& rotation : rotations)
			{
				rotationKeyFrames.emplace_back(rotation.second);
			}
			for (const auto& scale : scales)
			{
				scaleKeyFrames.emplace_back(scale.second);
			}

			return std::tuple{ translationKeyFrames, rotationKeyFrames, scaleKeyFrames };
		}


		Scope<Animation> ImportAnimation(const aiScene* scene, const uint32_t animationIndex, const Skeleton& skeleton, const bool isMaskedRootMotion, const glm::vec3& rootTranslationMask, float rootRotationMask)
		{
			if (!scene)
			{
				return nullptr;
			}

			if (animationIndex >= scene->mNumAnimations)
			{
				return nullptr;
			}

			const aiAnimation* anim = scene->mAnimations[animationIndex];
			auto channels = ImportChannels(anim, skeleton, isMaskedRootMotion, rootTranslationMask, rootRotationMask);
			auto [translationKeysTemp, rotationKeysTemp, scaleKeysTemp] = ConcatenateChannelsAndSort(channels);
			auto [translationKeys, rotationKeys, scaleKeys] = ExtractKeys(translationKeysTemp, rotationKeysTemp, scaleKeysTemp);

			double samplingRate = anim->mTicksPerSecond;
			if (samplingRate < 0.0001)
			{
				samplingRate = 1.0;
			}

			auto animation = CreateScope<Animation>(static_cast<float>(anim->mDuration / samplingRate), static_cast<uint32_t>(channels.size()));
			animation->SetKeys(std::move(translationKeys), std::move(rotationKeys), std::move(scaleKeys));
			return animation;
		}


		BoneHierarchy::BoneHierarchy(const aiScene* scene) : m_Scene(scene)
		{
		}


		Scope<Engine::Skeleton> BoneHierarchy::CreateSkeleton()
		{
			if (!m_Scene)
			{
				return nullptr;
			}

			ExtractBones();
			if (m_Bones.empty())
			{
				return nullptr;
			}

			auto skeleton = CreateScope<Skeleton>(static_cast<uint32_t>(m_Bones.size()));
			TraverseNode(m_Scene->mRootNode, skeleton.get());

			return skeleton;
		}


		void BoneHierarchy::ExtractBones()
		{
			// Note: ASSIMP does not appear to support import of digital content files that contain _only_ an armature/skeleton and no mesh.
			for (uint32_t meshIndex = 0; meshIndex < m_Scene->mNumMeshes; ++meshIndex)
			{
				const aiMesh* mesh = m_Scene->mMeshes[meshIndex];
				for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
				{
					m_Bones.emplace(mesh->mBones[boneIndex]->mName.C_Str());
				}
			}
		}


		void BoneHierarchy::TraverseNode(aiNode* node, Skeleton* skeleton)
		{
			if (m_Bones.find(node->mName.C_Str()) != m_Bones.end())
			{
				TraverseBone(node, skeleton, Skeleton::NullIndex);
			}
			else
			{
				for (uint32_t nodeIndex = 0; nodeIndex < node->mNumChildren; ++nodeIndex)
				{
					TraverseNode(node->mChildren[nodeIndex], skeleton);
				}
			}
		}


		void BoneHierarchy::TraverseBone(aiNode* node, Skeleton* skeleton, uint32_t parentIndex)
		{
			uint32_t boneIndex = skeleton->AddBone(node->mName.C_Str(), parentIndex, Utils::Mat4FromAIMatrix4x4(node->mTransformation));
			for (uint32_t nodeIndex = 0; nodeIndex < node->mNumChildren; ++nodeIndex)
			{
				TraverseBone(node->mChildren[nodeIndex], skeleton, boneIndex);
			}
		}

	}
}
