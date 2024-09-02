#include "hzpch.h"
#include "BlendNodes.h"

#include "Hazel/Animation/Animation.h"
#include "Hazel/Animation/BlendUtils.h"
#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Debug/Profiler.h"

#include <CDT/CDT.h>

namespace Hazel::AnimationGraph {

	BlendSpaceVertex::BlendSpaceVertex(const char* dbgName, UUID id)
	: NodeProcessor(dbgName, id)
	, m_TrackWriter(static_cast<Pose*>(out_Pose.getRawData()))
	{
		// Inputs
		EndpointUtilities::RegisterEndpoints(this);
	}


	void BlendSpaceVertex::Init(const Skeleton*)
	{
		EndpointUtilities::InitializeInputs(this);
		m_PreviousAnimation = 0;
		m_AnimationTimePos = 0.0f;
		m_PreviousAnimationTimePos = m_AnimationTimePos;

		Pose defaultPose;
		HZ_CORE_ASSERT(sizeof(Pose) == out_Pose.getRawDataSize());
		memcpy(out_Pose.getRawData(), &defaultPose, sizeof(Pose));
	}


	void BlendSpaceVertex::EnsureAnimation()
	{
		Pose* pose = static_cast<Pose*>(out_Pose.getRawData());
		if (m_PreviousAnimation != *in_Animation)
		{
			if(auto animationAsset = AssetManager::GetAsset<AnimationAsset>(*in_Animation); animationAsset)
			{
				m_Animation = animationAsset->GetAnimation();
			}
			if (m_Animation)
			{
				context.initialize(*static_cast<const acl::compressed_tracks*>(m_Animation->GetData()));
				m_RootTranslationStart = m_Animation->GetRootTranslationStart();
				m_RootRotationStart = m_Animation->GetRootRotationStart();
				m_RootTranslationEnd = m_Animation->GetRootTranslationEnd();
				m_RootRotationEnd = m_Animation->GetRootRotationEnd();
				pose->BoneTransforms[0].Translation = m_RootTranslationStart;
				pose->BoneTransforms[0].Rotation = m_RootRotationStart;
				pose->AnimationDuration = m_Animation->GetDuration();
				pose->NumBones = m_Animation->GetNumTracks();
			}
			else
			{
				pose->AnimationDuration = 1.0f;
				pose->NumBones = 0;
			}
			pose->AnimationTimePos = m_AnimationTimePos = 0.0f;
			m_PreviousAnimationTimePos = m_AnimationTimePos;
			m_PreviousAnimation = *in_Animation;
		}
	}


	void BlendSpaceVertex::UpdateTime(float timestep)
	{
		if (m_Animation)
		{
			m_AnimationTimePos += timestep / m_Animation->GetDuration();
			m_AnimationTimePos -= floorf(m_AnimationTimePos);
		}
	}


	float BlendSpaceVertex::Process(float timestep)
	{
		HZ_PROFILE_FUNC();
		if (m_Animation)
		{
			Pose* pose = static_cast<Pose*>(out_Pose.getRawData());

			glm::vec3 previousRootTranslation = pose->BoneTransforms[0].Translation;
			glm::quat previousRootRotation = pose->BoneTransforms[0].Rotation;

			m_AnimationTimePos += timestep / m_Animation->GetDuration();
			m_AnimationTimePos -= floorf(m_AnimationTimePos);

			context.seek(m_AnimationTimePos * pose->AnimationDuration, acl::sample_rounding_policy::none);
			context.decompress_tracks(m_TrackWriter);

			// Work out root motion by looking at change in pose of root bone.
			glm::vec3 newRootTranslation = pose->BoneTransforms[0].Translation;
			glm::quat newRootRotation = pose->BoneTransforms[0].Rotation;
			if (m_AnimationTimePos >= m_PreviousAnimationTimePos)
			{
				// Animation has not looped
				pose->RootMotion.Translation = newRootTranslation - previousRootTranslation;
				pose->RootMotion.Rotation = glm::conjugate(previousRootRotation) * newRootRotation;
			}
			else
			{
				// Animation has looped
				pose->RootMotion.Translation = m_RootTranslationEnd - previousRootTranslation + newRootTranslation - m_RootTranslationStart;
				pose->RootMotion.Rotation = (glm::conjugate(previousRootRotation) * m_RootRotationEnd) * (glm::conjugate(m_RootRotationStart) * newRootRotation);
			}

			m_PreviousAnimationTimePos = m_AnimationTimePos;
			pose->AnimationTimePos = m_AnimationTimePos;
		}
		return 0.0f;
	}


	float BlendSpaceVertex::GetAnimationDuration() const
	{
		return static_cast<const Pose*>(out_Pose.getRawData())->AnimationDuration;
	}


	float BlendSpaceVertex::GetAnimationTimePos() const
	{
		return m_AnimationTimePos;
	}


	void BlendSpaceVertex::SetAnimationTimePos(float timePos)
	{
		m_PreviousAnimationTimePos = timePos;
		m_AnimationTimePos = timePos;
	}


	BlendSpace::BlendSpace(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		// Inputs
		EndpointUtilities::RegisterEndpoints(this);
	}


	bool CompareCollinearVertices(const glm::vec2& reference, const BlendSpaceVertex* a, const BlendSpaceVertex* b)
	{
		double angleA = atan2(a->Y - reference.y, a->X - reference.x);
		double angleB = atan2(b->Y - reference.y, b->X - reference.x);
		return angleA < angleB;
	}


	void BlendSpace::Init(const Skeleton* skeleton)
	{
		EndpointUtilities::InitializeInputs(this);

		m_Skeleton = skeleton;

		for (auto& vertex : m_Vertices)
		{
			vertex->Init(skeleton);
		}

		if (m_Vertices.size() >= 3)
		{
			m_Triangulation = CDT::Triangulation<float>();
			m_Triangulation.insertVertices(m_Vertices.begin(), m_Vertices.end(), [](Scope<BlendSpaceVertex>& vertex) { return vertex->X; }, [](Scope<BlendSpaceVertex>& vertex) { return vertex->Y; });
			m_Triangulation.eraseSuperTriangle();

			if (m_Triangulation.triangles.size() == 0)
			{
				// Can get here if the vertices are collinear
				// Sort them in the order the fall along the line.
				// Later, in Process() we then project the blend space position to this line
				// and blend between the appropriate vertices depending upon which line
				// segment the projected position falls within.
				glm::vec2 reference = {m_Vertices[0]->X, m_Vertices[0]->Y};
				for (const auto& vertex : m_Vertices)
				{
					if (vertex->X < reference.x || (vertex->X == reference.x && vertex->Y < reference.y))
					{
						reference = { vertex->X, vertex->Y };
					}
				}
				std::sort(m_Vertices.begin(), m_Vertices.end(), [&](const auto& a, const auto& b) {
					return CompareCollinearVertices(reference, a.get(), b.get());
				});
			}
		}

		Pose defaultPose;
		HZ_CORE_ASSERT(sizeof(Pose) == out_Pose.getRawDataSize());
		memcpy(out_Pose.getRawData(), &defaultPose, sizeof(Pose));
	}


	void BlendSpace::LerpPosition(float timestep)
	{
		// Lots of more interesting things we could replace this with (e.g. ease in/out, spring dampening, etc.)
		// For now just a simple lerp.
		float diffX = m_LastX == -FLT_MAX? 0.0f : *in_X - m_LastX;
		float diffY = m_LastY == -FLT_MAX? 0.0f : *in_Y - m_LastY;

		auto durationX = glm::abs(diffX) * m_LerpSecondsPerUnitX;
		auto durationY = glm::abs(diffY) * m_LerpSecondsPerUnitY;

		if (durationX <= timestep)
		{
			m_LastX = *in_X;
		}
		else
		{
			m_LastX += diffX / durationX * timestep;
		}

		if (durationY <= timestep)
		{
			m_LastY = *in_Y;
		}
		else
		{
			m_LastY += diffY / durationY * timestep;
		}
	}


	void BlendSpace::Blend1(CDT::IndexSizeType i0, float timestep, CDT::IndexSizeType& previousI0, CDT::IndexSizeType& previousI1, CDT::IndexSizeType& previousI2)
	{
		HZ_PROFILE_FUNC();
		m_Vertices[i0]->EnsureAnimation();

		Pose* result = static_cast<Pose*>(out_Pose.getRawData());

		auto prevTimePos = result->AnimationTimePos;

		if ((m_PreviousI0 != i0) && (m_PreviousI1 != i0) && (m_PreviousI2 != i0))
		{
			// this vertex was not sampled previous frame. restart from timepos 0
			m_Vertices[i0]->SetAnimationTimePos(0.0f);
		}

		m_Vertices[i0]->Process(timestep);

		const Pose* pose0 = static_cast<const Pose*>(m_Vertices[i0]->out_Pose.getRawData());
		for (uint32_t i = 0, N = pose0->NumBones; i < N; i++)
		{
			result->BoneTransforms[i].Translation = pose0->BoneTransforms[i].Translation;
			result->BoneTransforms[i].Rotation = pose0->BoneTransforms[i].Rotation;
			result->BoneTransforms[i].Scale = pose0->BoneTransforms[i].Scale;
		}
		result->RootMotion.Translation = pose0->RootMotion.Translation;
		result->RootMotion.Rotation = pose0->RootMotion.Rotation;
		result->RootMotion.Scale = pose0->RootMotion.Scale;
		result->AnimationDuration = pose0->AnimationDuration;
		result->AnimationTimePos = pose0->AnimationTimePos;
		result->NumBones = pose0->NumBones;
		previousI0 = i0;
		previousI1 = -1;
		previousI2 = -1;
	}


	void BlendSpace::Blend2(CDT::IndexSizeType i0, CDT::IndexSizeType i1, float u, float v, float timestep, CDT::IndexSizeType& previousI0, CDT::IndexSizeType& previousI1, CDT::IndexSizeType& previousI2)
	{
		HZ_PROFILE_FUNC();
		HZ_CORE_ASSERT((u > 0) && (v > 0), "Both weights for Blend2 must be strictly positive"); // otherwise you aren't blending two things
		m_Vertices[i0]->EnsureAnimation();
		m_Vertices[i1]->EnsureAnimation();

		Pose* result = static_cast<Pose*>(out_Pose.getRawData());

		auto prevTimePos = result->AnimationTimePos;
		bool sync0 = *m_Vertices[i0]->in_Synchronize;
		bool sync1 = *m_Vertices[i1]->in_Synchronize;
		bool sync = sync0 && sync1;
		if ((m_PreviousI0 != i0) && (m_PreviousI1 != i0) && (m_PreviousI2 != i0))
		{
			// This vertex was not sampled in previous frame.
			// Either restart the animation from 0 or synch it up
			if (sync)
			{
				m_Vertices[i0]->SetAnimationTimePos(prevTimePos);
			}
			else
			{
				m_Vertices[i0]->SetAnimationTimePos(0.0f);
			}
		}
		if ((m_PreviousI0 != i1) && (m_PreviousI1 != i1) && (m_PreviousI2 != i1))
		{
			// This vertex was not sampled in previous frame.
			// Either restart the animation or synch it up
			if (sync)
			{
				m_Vertices[i1]->SetAnimationTimePos(prevTimePos);
			}
			else
			{
				m_Vertices[i1]->SetAnimationTimePos(0.0f);
			}
		}

		if (sync)
		{
			auto duration0 = m_Vertices[i0]->GetAnimationDuration();
			auto duration1 = m_Vertices[i1]->GetAnimationDuration();
			auto weightedDuration = u * duration0 + v * duration1;
			m_Vertices[i0]->Process(timestep * duration0 / weightedDuration);
			m_Vertices[i1]->Process(timestep * duration1 / weightedDuration);
			result->AnimationDuration = weightedDuration;
			result->AnimationTimePos = m_Vertices[i0]->GetAnimationTimePos(); // note: vertices are in sync. vertex0 animTimePos == vertex1 animTimePos
		}
		else
		{
			m_Vertices[i0]->Process(timestep);
			m_Vertices[i1]->Process(timestep);
			if (sync0)
			{
				result->AnimationDuration = m_Vertices[i0]->GetAnimationDuration();
				result->AnimationTimePos = m_Vertices[i0]->GetAnimationTimePos();
			}
			else if (sync1)
			{
				result->AnimationDuration = m_Vertices[i1]->GetAnimationDuration();
				result->AnimationTimePos = m_Vertices[i1]->GetAnimationTimePos();
			}
			else
			{
				// Return duration and pos of the sync'd vertex, or vertex0 if neither is sync'd
				result->AnimationDuration = sync1? m_Vertices[i1]->GetAnimationDuration() : m_Vertices[i0]->GetAnimationDuration();
				result->AnimationTimePos = sync1? m_Vertices[i1]->GetAnimationTimePos() : m_Vertices[i0]->GetAnimationTimePos();
			}
		}

		const Pose* pose0 = static_cast<const Pose*>(m_Vertices[i0]->out_Pose.getRawData());
		const Pose* pose1 = static_cast<const Pose*>(m_Vertices[i1]->out_Pose.getRawData());
		Utils::Animation::BlendBoneTransforms(pose0, pose1, v, m_Skeleton, 0, result);
		Utils::Animation::BlendRootMotion(pose0, pose1, v, result);

		result->NumBones = pose0->NumBones;
		previousI0 = i0;
		previousI1 = i1;
		previousI2 = -1;
	}


	void BlendSpace::Blend3(CDT::IndexSizeType i0, CDT::IndexSizeType i1, CDT::IndexSizeType i2, float u, float v, float w, float timestep)
	{
		HZ_PROFILE_FUNC();
		HZ_CORE_ASSERT((u > 0) && (v > 0) && (w > 0), "All weights for Blend3 must be strictly positive"); // otherwise you aren't blending three things
		m_Vertices[i0]->EnsureAnimation();
		m_Vertices[i1]->EnsureAnimation();
		m_Vertices[i2]->EnsureAnimation();

		Pose* result = static_cast<Pose*>(out_Pose.getRawData());
		auto prevTimePos = result->AnimationTimePos;

		bool sync0 = *m_Vertices[i0]->in_Synchronize;
		bool sync1 = *m_Vertices[i1]->in_Synchronize;
		bool sync2 = *m_Vertices[i2]->in_Synchronize;

		if ((m_PreviousI0 != i0) && (m_PreviousI1 != i0) && (m_PreviousI2 != i0))
		{
			sync0 = false;
		}
		if ((m_PreviousI0 != i1) && (m_PreviousI1 != i1) && (m_PreviousI2 != i1))
		{
			sync1 = false;
		}
		if ((m_PreviousI0 != i2) && (m_PreviousI1 != i2) && (m_PreviousI2 != i2))
		{
			sync2 = false;
		}

		if ((m_PreviousI0 != i0) && (m_PreviousI1 != i0) && (m_PreviousI2 != i0))
		{
			if (sync0 && (sync1 || sync2))
			{
				m_Vertices[i0]->SetAnimationTimePos(prevTimePos);
			}
			else
			{
				m_Vertices[i0]->SetAnimationTimePos(0.0f);
			}
		}
		if ((m_PreviousI0 != i1) && (m_PreviousI1 != i1) && (m_PreviousI2 != i1))
		{
			if (sync1 && (sync0 || sync2))
			{
				m_Vertices[i1]->SetAnimationTimePos(prevTimePos);
			}
			else
			{
				m_Vertices[i1]->SetAnimationTimePos(0.0f);
			}
		}
		if ((m_PreviousI0 != i2) && (m_PreviousI1 != i2) && (m_PreviousI2 != i2))
		{
			if (sync2 && (sync0 || sync1))
			{
				m_Vertices[i2]->SetAnimationTimePos(prevTimePos);
			}
			else
			{
				m_Vertices[i2]->SetAnimationTimePos(0.0f);
			}
		}

		if (sync0 && sync1 && sync2)
		{
			auto duration0 = m_Vertices[i0]->GetAnimationDuration();
			auto duration1 = m_Vertices[i1]->GetAnimationDuration();
			auto duration2 = m_Vertices[i2]->GetAnimationDuration();
			auto weightedDuration = u * duration0 + v * duration1 + w * duration2;
			m_Vertices[i0]->Process(timestep * duration0 / weightedDuration);
			m_Vertices[i1]->Process(timestep * duration1 / weightedDuration);
			m_Vertices[i2]->Process(timestep * duration2 / weightedDuration);
			result->AnimationDuration = weightedDuration;
			result->AnimationTimePos = m_Vertices[i0]->GetAnimationTimePos(); // note: vertices are in sync. vertex0 animTimePos == vertex1 animTimePos == vertex2 animTimePos
		}
		else if (sync0 && sync1)
		{
			auto duration0 = m_Vertices[i0]->GetAnimationDuration();
			auto duration1 = m_Vertices[i1]->GetAnimationDuration();
			auto weightedDuration = u * duration0 + v * duration1;
			m_Vertices[i0]->Process(timestep * duration0 / weightedDuration);
			m_Vertices[i1]->Process(timestep * duration1 / weightedDuration);
			m_Vertices[i2]->Process(timestep);
			result->AnimationDuration = weightedDuration;
			result->AnimationTimePos = m_Vertices[i0]->GetAnimationTimePos();
		}
		else if (sync0 && sync2)
		{
			auto duration0 = m_Vertices[i0]->GetAnimationDuration();
			auto duration2 = m_Vertices[i2]->GetAnimationDuration();
			auto weightedDuration = u * duration0 + w * duration2;
			m_Vertices[i0]->Process(timestep * duration0 / weightedDuration);
			m_Vertices[i1]->Process(timestep);
			m_Vertices[i2]->Process(timestep * duration2 / weightedDuration);
			result->AnimationDuration = weightedDuration;
			result->AnimationTimePos = m_Vertices[i0]->GetAnimationTimePos();
		}
		else if (sync1 && sync2)
		{
			auto duration1 = m_Vertices[i1]->GetAnimationDuration();
			auto duration2 = m_Vertices[i2]->GetAnimationDuration();
			auto weightedDuration = v * duration1 + w * duration2;
			m_Vertices[i0]->Process(timestep);
			m_Vertices[i1]->Process(timestep * duration1 / weightedDuration);
			m_Vertices[i2]->Process(timestep * duration2 / weightedDuration);
			result->AnimationDuration = weightedDuration;
			result->AnimationTimePos = m_Vertices[i1]->GetAnimationTimePos();
		}
		else
		{
			m_Vertices[i0]->Process(timestep);
			m_Vertices[i1]->Process(timestep);
			m_Vertices[i2]->Process(timestep);
			if (sync0)
			{
				result->AnimationDuration = m_Vertices[i0]->GetAnimationDuration();
				result->AnimationTimePos = m_Vertices[i0]->GetAnimationTimePos();
			}
			else if (sync1)
			{
				result->AnimationDuration = m_Vertices[i1]->GetAnimationDuration();
				result->AnimationTimePos = m_Vertices[i1]->GetAnimationTimePos();
			}
			else if (sync2)
			{
				result->AnimationDuration = m_Vertices[i2]->GetAnimationDuration();
				result->AnimationTimePos = m_Vertices[i2]->GetAnimationTimePos();
			}
			else
			{
				// it doesn't matter. arbitrarily choose vertex0
				result->AnimationDuration = m_Vertices[i0]->GetAnimationDuration();
				result->AnimationTimePos = m_Vertices[i0]->GetAnimationTimePos();
			}
		}


		const Pose* poseA = static_cast<const Pose*>(m_Vertices[i0]->out_Pose.getRawData());
		const Pose* poseB = static_cast<const Pose*>(m_Vertices[i1]->out_Pose.getRawData());
		const Pose* poseC = static_cast<const Pose*>(m_Vertices[i2]->out_Pose.getRawData());
		HZ_CORE_ASSERT((poseA->NumBones == poseB->NumBones) && (poseA->NumBones == poseC->NumBones), "Poses have different number of bones");

		auto v1 = v / (u + v);
		for (uint32_t i = 0, N = glm::min(glm::min(poseA->NumBones, poseB->NumBones), poseC->NumBones); i < N; i++)
		{
			result->BoneTransforms[i].Translation = glm::mix(glm::mix(poseA->BoneTransforms[i].Translation, poseB->BoneTransforms[i].Translation, v1), poseC->BoneTransforms[i].Translation, w);
			result->BoneTransforms[i].Rotation = glm::slerp(glm::slerp(poseA->BoneTransforms[i].Rotation, poseB->BoneTransforms[i].Rotation, v1), poseC->BoneTransforms[i].Rotation, w);
			result->BoneTransforms[i].Scale = glm::mix(glm::mix(poseA->BoneTransforms[i].Scale, poseB->BoneTransforms[i].Scale, v1), poseC->BoneTransforms[i].Scale, w);
		}
		result->RootMotion.Translation = glm::mix(glm::mix(poseA->RootMotion.Translation, poseB->RootMotion.Translation, v1), poseC->RootMotion.Translation, w);
		result->RootMotion.Rotation = glm::slerp(glm::slerp(poseA->RootMotion.Rotation, poseB->RootMotion.Rotation, v1), poseC->RootMotion.Rotation, w);
		result->RootMotion.Scale = glm::mix(glm::mix(poseA->RootMotion.Scale, poseB->RootMotion.Scale, v1), poseC->RootMotion.Scale, w);
		result->NumBones = poseA->NumBones;
		m_PreviousI0 = i0;
		m_PreviousI1 = i1;
		m_PreviousI2 = i2;
	}


	// returns {distanceSquared, t}
	std::tuple<float, float> ClosestPointToSegment(CDT::V2d<float> p, CDT::V2d<float> a, CDT::V2d<float> b)
	{
		const CDT::V2d<float> ab = { b.x - a.x, b.y - a.y };
		const CDT::V2d<float> ap = { p.x - a.x, p.y - a.y };

		const float ab2 = ab.x * ab.x + ab.y * ab.y;
		const float ap_ab = ap.x * ab.x + ap.y * ab.y;
		const float t = ap_ab / ab2;

		if (t < 0.0f)
			return { (a.x - p.x) * (a.x - p.x) + (a.y - p.y) * (a.y - p.y), 0.0f };
		else if (t > 1.0f)
			return { (b.x - p.x) * (b.x - p.x) + (b.y - p.y) * (b.y - p.y), 1.0f };

		return { (a.x + t * ab.x - p.x) * (a.x + t * ab.x - p.x) + (a.y + t * ab.y - p.y) * (a.y + t * ab.y - p.y), t };
	}


	// returns {distanceSquared, u, v, w}
	std::tuple<float, float, float, float> ClosestPointToTriangle(CDT::V2d<float> p, CDT::V2d<float> a, CDT::V2d<float> b, CDT::V2d<float> c)
	{
		const CDT::V2d<float> ab = { b.x - a.x, b.y - a.y };
		const CDT::V2d<float> ac = { c.x - a.x, c.y - a.y };
		const CDT::V2d<float> ap = { p.x - a.x, p.y - a.y };

		const float d1 = ab.x * ap.x + ab.y * ap.y;
		const float d2 = ac.x * ap.x + ac.y * ap.y;
		if (d1 <= 0.0f && d2 <= 0.0f) return { (a.x - p.x) * (a.x - p.x) + (a.y - p.y) * (a.y - p.y), 1.0f, 0.0f, 0.0f};

		const CDT::V2d<float> bp = { p.x - b.x, p.y - b.y };
		const float d3 = ab.x * bp.x + ab.y * bp.y;
		const float d4 = ac.x * bp.x + ac.y * bp.y;
		if (d3 >= 0.0f && d4 <= d3) return { (b.x - p.x) * (b.x - p.x) + (b.y - p.y) * (b.y - p.y), 0.0f, 1.0f, 0.0f };

		const CDT::V2d<float> cp = { p.x - c.x, p.y - c.y };
		const float d5 = ab.x * cp.x + ab.y * cp.y;
		const float d6 = ac.x * cp.x + ac.y * cp.y;
		if (d6 >= 0.0f && d5 <= d6) return { (c.x - p.x) * (c.x - p.x) + (c.y - p.y) * (c.y - p.y), 0.0f, 0.0f, 1.0f };

		const float vc = d1 * d4 - d3 * d2;
		if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
		{
			const float v = d1 / (d1 - d3);
			return { (a.x + v * ab.x - p.x) * (a.x + v * ab.x - p.x) + (a.y + v * ab.y - p.y) * (a.y + v * ab.y - p.y), 1.0f - v, v, 0.0f };
		}

		const float vb = d5 * d2 - d1 * d6;
		if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
		{
			const float v = d2 / (d2 - d6);
			return { (a.x + v * ac.x - p.x) * (a.x + v * ac.x - p.x) + (a.y + v * ac.y - p.y) * (a.y + v * ac.y - p.y), 1.0f - v, 0.0f, v };
		}

		const float va = d3 * d6 - d5 * d4;
		if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
		{
			const float v = (d4 - d3) / ((d4 - d3) + (d5 - d6));
			return { (b.x + v * (c.x - b.x) - p.x) * (b.x + v * (c.x - b.x) - p.x) + (b.y + v * (c.y - b.y) - p.y) * (b.y + v * (c.y - b.y) - p.y), 0.0f, 1.0f - v, v };
		}

		// We should never get here since we only call ClosestPointToTriangle for points that are outside of the triangle.
		// It is here for completeness only.
		const float denom = 1.f / (va + vb + vc);
		const float v = vb * denom;
		const float w = vc * denom;
		return { 0.0f, 1.0f - v - w, v, w };
	}


	float BlendSpace::Process(float timestep)
	{
		HZ_PROFILE_FUNC();

		if (m_Vertices.size() == 1)
		{
			Blend1(0, timestep, m_PreviousI0, m_PreviousI1, m_PreviousI2);
			return 0.0f;
		}

		if (m_Vertices.size() == 2)
		{
			if ((*in_X != m_LastX) || (*in_Y != m_LastY))
			{
				LerpPosition(timestep);

				// calculate projection of position onto line segment
				// formed by vertices 0 and 1.
				glm::vec2 v0 = { m_Vertices[0]->X, m_Vertices[0]->Y };
				glm::vec2 v1 = { m_Vertices[1]->X, m_Vertices[1]->Y };
				glm::vec2 v = v1 - v0;
				glm::vec2 w = { m_LastX - v0.x, m_LastY - v0.y };

				m_U = glm::dot(w, v) / glm::dot(v, v);
			}

			if (m_U <= 0.0f)
			{
				Blend1(0, timestep, m_PreviousI0, m_PreviousI1, m_PreviousI2);
				return 0.0f;
			}
			if (m_U >= 1.0f)
			{
				Blend1(1, timestep, m_PreviousI1, m_PreviousI2, m_PreviousI0);
				return 0.0f;
			}
			Blend2(0, 1, 1.0f - m_U, m_U, timestep, m_PreviousI0, m_PreviousI1, m_PreviousI2);
			return 0.0f;
		}

		// For each triangle in m_Triangulation,
		// Compute the barycentric coorindates of the point (*in_X, *in_Y).
		// If all barycentric coordinates are >= 0, then the point is inside the triangle.
		// If the point is inside the triangle, then interpolate the poses of the three vertices of the triangle
		// according to the barycentric coordinates, and return the interpolated pose.
		auto i0 = m_PreviousI0;
		auto i1 = m_PreviousI1;
		auto i2 = m_PreviousI2;
		if ((*in_X != m_LastX) || (*in_Y != m_LastY))
		{
			LerpPosition(timestep);
			m_U = 0.0f;
			m_V = 0.0f;
			m_W = 0.0f;
			for (auto triangle : m_Triangulation.triangles)
			{
				i0 = triangle.vertices[0];
				i1 = triangle.vertices[1];
				i2 = triangle.vertices[2];

				auto v0 = m_Triangulation.vertices[i0];
				auto v1 = m_Triangulation.vertices[i1];
				auto v2 = m_Triangulation.vertices[i2];

				auto det = ((v1.y - v2.y) * (v0.x - v2.x) + (v2.x - v1.x) * (v0.y - v2.y));
				m_U = ((v1.y - v2.y) * (m_LastX - v2.x) + (v2.x - v1.x) * (m_LastY - v2.y)) / det;
				m_V = ((v2.y - v0.y) * (m_LastX - v2.x) + (v0.x - v2.x) * (m_LastY - v2.y)) / det;
				m_W = 1.0f - m_U - m_V;

				if ((m_U > 0.0f) && (m_V > 0.0f) && (m_W > 0.0f))
				{
					Blend3(i0, i1, i2, m_U, m_V, m_W, timestep);
					return 0.0f;
				}
			}

			// If we get here, then the point is outside all triangles (and note: there could be zero triangles)
			if (!m_Triangulation.triangles.empty())
			{
				// Do another sweep to find which triangle is closest to the point
				// and then interpolate accordingly.
				float minDistanceSquared = FLT_MAX;
				CDT::IndexSizeType closestTriangle = 0;
				for (auto triangle : m_Triangulation.triangles)
				{
					auto j0 = triangle.vertices[0];
					auto j1 = triangle.vertices[1];
					auto j2 = triangle.vertices[2];

					auto v0 = m_Triangulation.vertices[j0];
					auto v1 = m_Triangulation.vertices[j1];
					auto v2 = m_Triangulation.vertices[j2];

					auto [distanceSquared, u, v, w] = ClosestPointToTriangle({ m_LastX, m_LastY }, v0, v1, v2);
					if (distanceSquared < minDistanceSquared)
					{
						minDistanceSquared = distanceSquared;
						i0 = j0;
						i1 = j1;
						i2 = j2;
						m_U = u;
						m_V = v;
						m_W = w;
					}
				}
			}
			else
			{
				// No triangles => all vertices in the blend space are co-linear
				// Do a sweep to which line segment is closest to the point
				float minDistanceSquared = FLT_MAX;
				float tMin = 0.0f;
				CDT::IndexSizeType closestSegment = 0;
				for (CDT::IndexSizeType i = 0; i < static_cast<CDT::IndexSizeType>(m_Vertices.size()) - 1; ++i)
				{
					CDT::V2d v0 = { m_Vertices[i]->X, m_Vertices[i]->Y };
					CDT::V2d v1 = { m_Vertices[i + 1]->X, m_Vertices[i + 1]->Y };

					auto [distanceSquared, t] = ClosestPointToSegment({ m_LastX, m_LastY }, v0, v1);
					if (distanceSquared < minDistanceSquared)
					{
						minDistanceSquared = distanceSquared;
						i0 = i;
						i1 = i + 1;
						tMin = t;
					}
				}
				if (tMin <= 0.0f)
				{
					m_U = 1.0f;
					m_V = 0.0f;
					m_W = 0.0f;
				}
				else if (tMin >= 1.0f)
				{
					m_U = 0.0f;
					m_V = 1.0f;
					m_W = 0.0f;
				}
				else
				{
					m_U = 1.0f - tMin;
					m_V = tMin;
					m_W = 0;
				}
			}
		}

		if ((m_U > 0.0f) && (m_V > 0.0f) && (m_W > 0.0f))
		{
			Blend3(i0, i1, i2, m_U, m_V, m_W, timestep);
			return 0.0f;
		}


		if ((m_U > 0.0f) && (m_V > 0.0f))
		{
			Blend2(i0, i1, m_U, m_V, timestep, m_PreviousI0, m_PreviousI1, m_PreviousI2);
			return 0.0f;
		}

		if ((m_U > 0.0f) && (m_W > 0.0f))
		{
			Blend2(i0, i2, m_U, m_W, timestep, m_PreviousI0, m_PreviousI2, m_PreviousI1);
			return 0.0f;
		}

		if ((m_V > 0.0f) && (m_W > 0.0f))
		{
			Blend2(i1, i2, m_V, m_W, timestep, m_PreviousI1, m_PreviousI2, m_PreviousI0);
			return 0.0f;
		}

		if (m_U > 0.0f)
		{
			Blend1(i0, timestep, m_PreviousI0, m_PreviousI1, m_PreviousI2);
			return 0.0f;
		}

		if (m_V > 0.0f)
		{
			Blend1(i1, timestep, m_PreviousI1, m_PreviousI2, m_PreviousI0);
			return 0.0f;
		}

		if (m_W > 0.0f)
		{
			Blend1(i2, timestep, m_PreviousI2, m_PreviousI0, m_PreviousI1);
			return 0.0f;
		}

		HZ_CORE_ASSERT(false); // should not be possible to get here. at least one of u, v, w must be > 0
		return 0.0f;
	}


	float BlendSpace::GetAnimationDuration() const
	{
		const Pose* result = static_cast<const Pose*>(out_Pose.getRawData());
		return result->AnimationDuration;
	}


	float BlendSpace::GetAnimationTimePos() const
	{
		const Pose* result = static_cast<const Pose*>(out_Pose.getRawData());
		return result->AnimationTimePos;
	}


	void BlendSpace::SetAnimationTimePos(float timePos)
	{
		for (auto& vertex : m_Vertices)
		{
			if (*vertex->in_Synchronize) {
				vertex->SetAnimationTimePos(timePos);
			}
			else
			{
				vertex->SetAnimationTimePos(0.0f);
			}
		}
	}


	ConditionalBlend::ConditionalBlend(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}


	void ConditionalBlend::Init(const Skeleton* skeleton)
	{
		HZ_CORE_ASSERT(m_AnimationGraph, "No animation graph set");
		EndpointUtilities::InitializeInputs(this);
		m_Skeleton = skeleton;
		m_AnimationGraph->Init();
	}


	float ConditionalBlend::Process(float timestep)
	{
		HZ_PROFILE_FUNC();
		HZ_CORE_ASSERT(m_AnimationGraph, "No animation graph set");

		if (*in_Condition)
		{
			m_TransitionWeight = *in_BlendInDuration < 0.0000001f ? 1.0f : glm::clamp(m_TransitionWeight + timestep / *in_BlendInDuration, 0.0f, 1.0f);
			m_AnimationGraph->Process(timestep);
		}
		else
		{
			m_TransitionWeight = *in_BlendOutDuration < 0.0000001f ? 0.0f : glm::clamp(m_TransitionWeight - timestep / *in_BlendOutDuration, 0.0f, 1.0f);
		}

		Pose* result = static_cast<Pose*>(out_Pose.getRawData());

		float w = m_TransitionWeight * *in_Weight;
		if(w < 0.0000001f)
		{
			memcpy(result, in_BasePose, sizeof(Pose));
		}
		else
		{
			if (*in_Additive)
			{
				Utils::Animation::AdditiveBlendBoneTransforms(in_BasePose, m_AnimationGraph->GetPose(), w, m_Skeleton, *in_BlendRootBone, result);

				if (*in_BlendRootBone == 0)
				{
					Utils::Animation::AdditiveBlendRootMotion(in_BasePose, m_AnimationGraph->GetPose(), w, result);
				}
				else
				{
					result->RootMotion = in_BasePose->RootMotion;
				}
			}
			else
			{
				Utils::Animation::BlendBoneTransforms(in_BasePose, m_AnimationGraph->GetPose(), w, m_Skeleton, *in_BlendRootBone, result);

				if (*in_BlendRootBone == 0)
				{
					Utils::Animation::BlendRootMotion(in_BasePose, m_AnimationGraph->GetPose(), w, result);
				}
				else
				{
					result->RootMotion = in_BasePose->RootMotion;
				}
			}
			result->AnimationDuration = in_BasePose->AnimationDuration;
			result->AnimationTimePos = in_BasePose->AnimationTimePos;
			result->NumBones = in_BasePose->NumBones;
		}
		return 0.0f;
	}


	OneShot::OneShot(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
	{
		AddInEvent(IDs::Trigger, [this](Identifier) { m_Trigger.SetDirty(); });
		EndpointUtilities::RegisterEndpoints(this);
	}


	void OneShot::Init(const Skeleton* skeleton)
	{
		HZ_CORE_ASSERT(m_AnimationGraph, "No animation graph set");
		EndpointUtilities::InitializeInputs(this);
		m_Skeleton = skeleton;
		m_AnimationGraph->Init();
	}


	float OneShot::Process(float timestep)
	{
		HZ_PROFILE_FUNC();
		HZ_CORE_ASSERT(m_AnimationGraph, "No animation graph set");

		float lastTimePos = 0.0f;
		if (m_Triggered)
		{
			lastTimePos = m_AnimationGraph->GetAnimationTimePos();
		}

		if (m_Trigger.CheckAndResetIfDirty())
		{
			if (!m_Triggered)
			{
				m_Triggered = true;
				m_TriggeredDuration = 0.0f;
				m_AnimationGraph->SetAnimationTimePos(0.0f);
			}
		}

		if (m_Triggered)
		{
			m_TriggeredDuration += timestep;
			auto duration = m_AnimationGraph->GetPose()->AnimationDuration;
			if ((duration > 0.0f) && (m_TriggeredDuration > duration))
			{
				out_OnFinished(IDs::Finished);
				m_Triggered = false;
			}
			else
			{
				m_AnimationGraph->Process(timestep);
			}

		}

		float w = 0.0f;
		if (m_Triggered)
		{
			if (m_TriggeredDuration < *in_BlendInDuration)
			{
				w = m_TriggeredDuration * *in_Weight / *in_BlendInDuration;
			}
			else
			{
				auto duration = m_AnimationGraph->GetPose()->AnimationDuration;
				if (m_TriggeredDuration > (duration - *in_BlendOutDuration))
				{
					w = (duration - m_TriggeredDuration) * *in_Weight / *in_BlendOutDuration;
				}
				else
				{
					w = *in_Weight;
				}
			}
		}
		Pose* result = static_cast<Pose*>(out_Pose.getRawData());
		if (w < 0.0000001f)
		{
			memcpy(result, in_BasePose, sizeof(Pose));
		}
		else
		{
			if (*in_Additive)
			{
				Utils::Animation::AdditiveBlendBoneTransforms(in_BasePose, m_AnimationGraph->GetPose(), w, m_Skeleton, *in_BlendRootBone, result);

				if (*in_BlendRootBone == 0)
				{
					Utils::Animation::AdditiveBlendRootMotion(in_BasePose, m_AnimationGraph->GetPose(), w, result);
				}
				else
				{
					result->RootMotion = in_BasePose->RootMotion;
				}
			}
			else
			{
				Utils::Animation::BlendBoneTransforms(in_BasePose, m_AnimationGraph->GetPose(), w, m_Skeleton, *in_BlendRootBone, result);

				if (*in_BlendRootBone == 0)
				{
					Utils::Animation::BlendRootMotion(in_BasePose, m_AnimationGraph->GetPose(), w, result);
				}
				else
				{
					result->RootMotion = in_BasePose->RootMotion;
				}
			}
			result->AnimationDuration = in_BasePose->AnimationDuration;
			result->AnimationTimePos = in_BasePose->AnimationTimePos;
			result->NumBones = in_BasePose->NumBones;
		}
		return 0.0f;
	}


	PoseBlend::PoseBlend(const char* dbgName, UUID id)
		: NodeProcessor(dbgName, id)
	{
		EndpointUtilities::RegisterEndpoints(this);
	}


	void PoseBlend::Init(const Skeleton* skeleton)
	{
		EndpointUtilities::InitializeInputs(this);
		m_Skeleton = skeleton;

		HZ_CORE_ASSERT(sizeof(Pose) == out_Pose.getRawDataSize());
		memcpy(out_Pose.getRawData(), &DefaultPose, sizeof(Pose));
	}


	float PoseBlend::Process(float timestep)
	{
		HZ_PROFILE_FUNC();

		Pose* result = static_cast<Pose*>(out_Pose.getRawData());
		if(*in_Additive) {
			Utils::Animation::AdditiveBlendBoneTransforms(in_PoseA, in_PoseB, *in_Alpha, m_Skeleton, *in_Bone, result);
			Utils::Animation::AdditiveBlendRootMotion(in_PoseA, in_PoseB, *in_Alpha, result);
		}
		else {
			Utils::Animation::BlendBoneTransforms(in_PoseA, in_PoseB, *in_Alpha, m_Skeleton, *in_Bone, result);
			Utils::Animation::BlendRootMotion(in_PoseA, in_PoseB, *in_Alpha, result);
		}
		result->AnimationDuration = 0.0f;
		result->AnimationTimePos = 0.0f;
		result->NumBones = in_PoseA->NumBones;

		return 0.0f;
	}


	RangedBlend::RangedBlend(const char* dbgName, UUID id)
	: NodeProcessor(dbgName, id)
	, m_TrackWriterA(static_cast<Pose*>(&m_PoseA))
	, m_TrackWriterB(static_cast<Pose*>(&m_PoseB))
	{
		EndpointUtilities::RegisterEndpoints(this);
	}


	void RangedBlend::Init(const Skeleton* skeleton)
	{
		EndpointUtilities::InitializeInputs(this);
		m_PreviousAnimationA = 0;
		m_PreviousAnimationB = 0;
		m_AnimationTimePosA = *in_OffsetA;
		m_AnimationTimePosB = *in_OffsetB;
		m_PreviousAnimationTimePosA = m_AnimationTimePosA;
		m_PreviousAnimationTimePosB = m_AnimationTimePosB;
		m_Skeleton = skeleton;

		Pose defaultPose;
		HZ_CORE_ASSERT(sizeof(Pose) == out_Pose.getRawDataSize());
		memcpy(out_Pose.getRawData(), &defaultPose, sizeof(Pose));
	}


	float RangedBlend::Process(float timestep)
	{
		HZ_PROFILE_FUNC();

		float w = glm::clamp((*in_Value - *in_RangeA) / (*in_RangeB - *in_RangeA), 0.0f, 1.0f);
		float weightedDuration = 0.0f;
		float weightedPlaybackSpeed = 0.0f;

		if (m_PreviousAnimationA != *in_AnimationA)
		{
			if(auto animationAsset = AssetManager::GetAsset<AnimationAsset>(*in_AnimationA); animationAsset)
			{
				m_AnimationA = animationAsset->GetAnimation();
			}
			if (m_AnimationA)
			{
				contextA.initialize(*static_cast<const acl::compressed_tracks*>(m_AnimationA->GetData()));
				m_RootTranslationStartA = m_AnimationA->GetRootTranslationStart();
				m_RootRotationStartA = m_AnimationA->GetRootRotationStart();
				m_RootTranslationEndA = m_AnimationA->GetRootTranslationEnd();
				m_RootRotationEndA = m_AnimationA->GetRootRotationEnd();
				m_PoseA.BoneTransforms[0].Translation = m_RootTranslationStartA;
				m_PoseA.BoneTransforms[0].Rotation = m_RootRotationStartA;
				m_PoseA.AnimationDuration = m_AnimationA->GetDuration();
				m_PoseA.NumBones = m_AnimationA->GetNumTracks();
			}
			m_AnimationTimePosA = *in_OffsetA;
			m_PreviousAnimationTimePosA = m_AnimationTimePosA;
			m_PreviousAnimationA = *in_AnimationA;
			m_IsFinishedA = false;
		}

		if (m_PreviousAnimationB != *in_AnimationB)
		{
			if(auto animationAsset = AssetManager::GetAsset<AnimationAsset>(*in_AnimationB); animationAsset)
			{
				m_AnimationB = animationAsset->GetAnimation();
			}
			if (m_AnimationB)
			{
				contextB.initialize(*static_cast<const acl::compressed_tracks*>(m_AnimationB->GetData()));
				m_RootTranslationStartB = m_AnimationB->GetRootTranslationStart();
				m_RootRotationStartB = m_AnimationB->GetRootRotationStart();
				m_RootTranslationEndB = m_AnimationB->GetRootTranslationEnd();
				m_RootRotationEndB = m_AnimationB->GetRootRotationEnd();
				m_PoseB.BoneTransforms[0].Translation = m_RootTranslationStartB;
				m_PoseB.BoneTransforms[0].Rotation = m_RootRotationStartB;
				m_PoseB.AnimationDuration = m_AnimationB->GetDuration();
				m_PoseB.NumBones = m_AnimationB->GetNumTracks();
			}
			m_AnimationTimePosB = *in_OffsetB;
			m_PreviousAnimationTimePosB = m_AnimationTimePosB;
			m_PreviousAnimationB = *in_AnimationB;
			m_IsFinishedB = false;
		}

		if (m_AnimationA && m_AnimationB)
		{
			HZ_CORE_ASSERT(m_AnimationA->GetNumTracks() == m_AnimationB->GetNumTracks());

			glm::vec3 previousRootTranslationA = m_PoseA.BoneTransforms[0].Translation;
			glm::quat previousRootRotationA = m_PoseA.BoneTransforms[0].Rotation;

			if (*in_Synchronize)
			{
				// When synchronized, this node's duration is the weighted average of the two animations,
				// and the two animations play in-synch. (i.e each update is to same time pos)
				weightedDuration = glm::mix(m_AnimationA->GetDuration(), m_AnimationB->GetDuration(), w);
				weightedPlaybackSpeed = glm::mix(*in_PlaybackSpeedA, *in_PlaybackSpeedB, w);
			}
			else
			{
				// When not synchronized, this node's duration is the duration of the first animation.
				// (So you can synch another node to the first animation)
				// The two animations play independently.
				weightedDuration = m_AnimationA->GetDuration();
				weightedPlaybackSpeed = *in_PlaybackSpeedA;
			}
				
			if (!m_IsFinishedA)
			{
				// start offset is 0 = beginning of anim, 1 = end of anim.
				// wrapping outside of [0, 1] due to offset changing is allowed, and does not ping looped events.
				m_AnimationTimePosA += *in_OffsetA - m_PreviousOffsetA;
				m_PreviousOffsetA = *in_OffsetA;
				m_AnimationTimePosA -= floorf(m_AnimationTimePosA);

				// timestep is in seconds, and needs to be normalized time value to the animation duration.
				m_AnimationTimePosA += timestep * weightedPlaybackSpeed / weightedDuration;

				if (*in_PlaybackSpeedA >= 0.0f)
				{
					while (m_AnimationTimePosA > 1.0f)
					{
						if (*in_LoopA)
						{
							m_AnimationTimePosA -= 1.0f;
							out_OnLoopA(IDs::LoopA);
						}
						else
						{
							m_AnimationTimePosA = 1.0f;
							m_IsFinishedA = true;
							out_OnFinishA(IDs::FinishA);
						}
					}
				}
				else
				{
					while (m_AnimationTimePosA < 0.0f)
					{
						if (*in_LoopA)
						{
							m_AnimationTimePosA += 1.0f;
							out_OnLoopA(IDs::LoopA);
						}
						else
						{
							m_AnimationTimePosA = 0.0f;
							m_IsFinishedA = true;
							out_OnFinishA(IDs::FinishA);
						}
					}
				}
			}
			contextA.seek(m_AnimationTimePosA * m_PoseA.AnimationDuration, acl::sample_rounding_policy::none);
			contextA.decompress_tracks(m_TrackWriterA);

			// Work out root motion by looking at change in pose of root bone.
			// Bear in mind some tricky cases:
			// 1) The animation might have looped back around to the beginning.
			// 2) We might be playing the animation backwards.
			// 3) We might have paused the animation, and be "debugging" it, stepping backwards (or forwards) deliberately.
			glm::vec3 newRootTranslation = m_PoseA.BoneTransforms[0].Translation;
			glm::quat newRootRotation = m_PoseA.BoneTransforms[0].Rotation;
			if (*in_PlaybackSpeedA >= 0.0f)
			{
				if (m_AnimationTimePosA >= m_PreviousAnimationTimePosA)
				{
					// Animation playing forwards, and has not looped
					m_PoseA.RootMotion.Translation = newRootTranslation - previousRootTranslationA;
					m_PoseA.RootMotion.Rotation = glm::conjugate(previousRootRotationA) * newRootRotation;
				}
				else
				{
					// Animation playing forwards, and has looped
					m_PoseA.RootMotion.Translation = m_RootTranslationEndA - previousRootTranslationA + newRootTranslation - m_RootTranslationStartA;
					m_PoseA.RootMotion.Rotation = (glm::conjugate(previousRootRotationA) * m_RootRotationEndA) * (glm::conjugate(m_RootRotationStartA) * newRootRotation);
				}
			}
			else
			{
				if (m_AnimationTimePosA <= m_PreviousAnimationTimePosA)
				{
					// Animation playing backwards, and has not looped
					m_PoseA.RootMotion.Translation = newRootTranslation - previousRootTranslationA;
					m_PoseA.RootMotion.Rotation = glm::conjugate(previousRootRotationA) * newRootRotation;
				}
				else
				{
					// Animation playing backwards, and has looped
					m_PoseA.RootMotion.Translation = m_RootTranslationStartA - previousRootTranslationA + newRootTranslation - m_RootTranslationEndA;
					m_PoseA.RootMotion.Rotation = (glm::conjugate(previousRootRotationA) * m_RootRotationStartA) * (glm::conjugate(m_RootRotationEndA) * newRootRotation);
				}
			}
			m_PreviousAnimationTimePosA = m_AnimationTimePosA;

			glm::vec3 previousRootTranslationB = m_PoseB.BoneTransforms[0].Translation;
			glm::quat previousRootRotationB = m_PoseB.BoneTransforms[0].Rotation;

			if (!m_IsFinishedB)
			{
				// start offset is 0 = beginning of anim, 1 = end of anim.
				// wrapping outside of [0, 1] due to offset changing is allowed, and does not ping looped events.
				m_AnimationTimePosB += *in_OffsetB - m_PreviousOffsetB;
				m_PreviousOffsetB = *in_OffsetB;
				m_AnimationTimePosB -= floorf(m_AnimationTimePosB);

				if (*in_Synchronize)
				{
					m_AnimationTimePosB = m_AnimationTimePosA;
				}
				else
				{
					// timestep is in seconds, and needs to be normalized time value to the animation duration.
					m_AnimationTimePosB += timestep * *in_PlaybackSpeedB / m_AnimationB->GetDuration();
				}

				if (*in_PlaybackSpeedB >= 0.0f)
				{
					while (m_AnimationTimePosB > 1.0f)
					{
						if (*in_LoopB)
						{
							m_AnimationTimePosB -= 1.0f;
							out_OnLoopB(IDs::LoopB);
						}
						else
						{
							m_AnimationTimePosB = 1.0f;
							m_IsFinishedB = true;
							out_OnFinishB(IDs::FinishB);
						}
					}
				}
				else
				{
					while (m_AnimationTimePosB < 0.0f)
					{
						if (*in_LoopB)
						{
							m_AnimationTimePosB += 1.0f;
							out_OnLoopB(IDs::LoopB);
						}
						else
						{
							m_AnimationTimePosB = 0.0f;
							m_IsFinishedB = true;
							out_OnFinishB(IDs::FinishB);
						}
					}
				}
			}
			contextB.seek(m_AnimationTimePosB * m_PoseB.AnimationDuration, acl::sample_rounding_policy::none);
			contextB.decompress_tracks(m_TrackWriterB);

			// Work out root motion by looking at change in pose of root bone.
			// Bear in mind some tricky cases:
			// 1) The animation might have looped back around to the beginning.
			// 2) We might be playing the animation backwards.
			// 3) We might have paused the animation, and be "debugging" it, stepping backwards (or forwards) deliberately.
			newRootTranslation = m_PoseB.BoneTransforms[0].Translation;
			newRootRotation = m_PoseB.BoneTransforms[0].Rotation;
			if (*in_PlaybackSpeedB >= 0.0f)
			{
				if (m_AnimationTimePosB >= m_PreviousAnimationTimePosB)
				{
					// Animation playing forwards, and has not looped
					m_PoseB.RootMotion.Translation = newRootTranslation - previousRootTranslationB;
					m_PoseB.RootMotion.Rotation = glm::conjugate(previousRootRotationB) * newRootRotation;
				}
				else
				{
					// Animation playing forwards, and has looped
					m_PoseB.RootMotion.Translation = m_RootTranslationEndB - previousRootTranslationB + newRootTranslation - m_RootTranslationStartB;
					m_PoseB.RootMotion.Rotation = (glm::conjugate(previousRootRotationB) * m_RootRotationEndB) * (glm::conjugate(m_RootRotationStartB) * newRootRotation);
				}
			}
			else
			{
				if (m_AnimationTimePosB <= m_PreviousAnimationTimePosB)
				{
					// Animation playing backwards, and has not looped
					m_PoseB.RootMotion.Translation = newRootTranslation - previousRootTranslationB;
					m_PoseB.RootMotion.Rotation = glm::conjugate(previousRootRotationB) * newRootRotation;
				}
				else
				{
					// Animation playing backwards, and has looped
					m_PoseB.RootMotion.Translation = m_RootTranslationStartB - previousRootTranslationB + newRootTranslation - m_RootTranslationEndB;
					m_PoseB.RootMotion.Rotation = (glm::conjugate(previousRootRotationB) * m_RootRotationStartB) * (glm::conjugate(m_RootRotationEndB) * newRootRotation);
				}
			}
			m_PreviousAnimationTimePosB = m_AnimationTimePosB;
		}

		Pose* result = static_cast<Pose*>(out_Pose.getRawData());
		Utils::Animation::BlendBoneTransforms(&m_PoseA, &m_PoseB, w, m_Skeleton, 0, result);
		Utils::Animation::BlendRootMotion(&m_PoseA, &m_PoseB, w, result);
		result->AnimationDuration = weightedDuration;
		result->AnimationTimePos = m_AnimationTimePosA; // Note that if the animations are sync'd then A.AnimationTimePos == B.AnimationTimePos, or if they are unsync'd its A that we want.
		result->NumBones = m_PoseA.NumBones;

		return 0.0f;
	}


	void RangedBlend::SetAnimationTimePos(float timePos)
	{
		if (*in_Synchronize)
		{
			// Synchronized.  Both animations get set to same timepos
			m_PreviousAnimationTimePosA = timePos;
			m_PreviousAnimationTimePosB = timePos;
			m_AnimationTimePosA = timePos;
			m_AnimationTimePosB = timePos;
		}
		else
		{
			// Unsynced.  Second animation always starts at 0.
			m_PreviousAnimationTimePosA = timePos;
			m_AnimationTimePosA = timePos;
			m_PreviousAnimationTimePosB = 0;
			m_AnimationTimePosB = 0;
		}
		m_IsFinishedA = false;
		m_IsFinishedB = false;
	}

}
