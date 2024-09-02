#pragma once

#include "Animation.h"

#include <acl/core/sample_rounding_policy.h>
#include <acl/core/track_writer.h>

#include <glm/gtc/type_ptr.hpp>

namespace Hazel {
	struct PoseTrackWriter : public acl::track_writer
	{
		PoseTrackWriter(Pose* pose) : m_Pose(pose) {}

		constexpr bool skip_track_scale(uint32_t track_index) const { return track_index == 0; }

		void RTM_SIMD_CALL write_rotation(uint32_t track_index, rtm::quatf_arg0 rotation)
		{
			rtm::quat_store(rotation, glm::value_ptr(m_Pose->BoneTransforms[track_index].Rotation));
		}

		void RTM_SIMD_CALL write_translation(uint32_t track_index, rtm::vector4f_arg0 translation)
		{
			rtm::vector_store3(translation, glm::value_ptr(m_Pose->BoneTransforms[track_index].Translation));
		}

		void RTM_SIMD_CALL write_scale(uint32_t track_index, rtm::vector4f_arg0 scale)
		{
			rtm::vector_store3(scale, glm::value_ptr(m_Pose->BoneTransforms[track_index].Scale));
		}

	private:
		Pose* m_Pose;
	};

}
