#include "math.h"

#include <glm/gtx/matrix_decompose.hpp>

namespace zong
{
namespace core
{

bool DecomposeTransform(glm::mat4 const& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale)
{
    // from glm::decompose in matrix_decompose.inl
    using T = float;

    glm::mat4 localMatrix(transform);

    // normalize the matrix.
    if (glm::epsilonEqual(localMatrix[3][3], static_cast<float>(0), glm::epsilon<T>()))
        return false;

    // first, isolate perspective.  This is the messiest.
    if (glm::epsilonNotEqual(localMatrix[0][3], static_cast<T>(0), glm::epsilon<T>()) ||
        glm::epsilonNotEqual(localMatrix[1][3], static_cast<T>(0), glm::epsilon<T>()) ||
        glm::epsilonNotEqual(localMatrix[2][3], static_cast<T>(0), glm::epsilon<T>()))
    {
        // clear the perspective partition
        localMatrix[0][3] = localMatrix[1][3] = localMatrix[2][3] = static_cast<T>(0);
        localMatrix[3][3]                                         = static_cast<T>(1);
    }

    // next take care of translation (easy).
    translation    = glm::vec3(localMatrix[3]);
    localMatrix[3] = glm::vec4(0, 0, 0, localMatrix[3].w);

    glm::vec3 Row[3], Pdum3;

    // now get scale and shear.
    for (glm::length_t i = 0; i < 3; ++i)
        for (glm::length_t j = 0; j < 3; ++j)
            Row[i][j] = localMatrix[i][j];

    // compute X scale factor and normalize first row.
    scale.x = length(Row[0]);
    Row[0]  = glm::detail::scale(Row[0], static_cast<T>(1));
    scale.y = length(Row[1]);
    Row[1]  = glm::detail::scale(Row[1], static_cast<T>(1));
    scale.z = length(Row[2]);
    Row[2]  = glm::detail::scale(Row[2], static_cast<T>(1));

    // at this point, the matrix (in rows[]) is orthonormal.
    // check for a coordinate system flip.  If the determinant
    // is -1, then negate the matrix and the scaling factors.
#if 0
		Pdum3 = cross(Row[1], Row[2]); // v3Cross(row[1], row[2], Pdum3);
		if (dot(Row[0], Pdum3) < 0)
		{
			for (length_t i = 0; i < 3; i++)
			{
				scale[i] *= static_cast<T>(-1);
				Row[i] *= static_cast<T>(-1);
			}
		}
#endif

    rotation.y = asin(-Row[0][2]);
    if (cos(rotation.y) != static_cast<float>(0.0)) // TODO: unsafe to judge float != float
    {
        rotation.x = atan2(Row[1][2], Row[2][2]);
        rotation.z = atan2(Row[0][1], Row[0][0]);
    }
    else
    {
        rotation.x = atan2(-Row[2][0], Row[1][1]);
        rotation.z = 0;
    }

    return true;
}

} // namespace core
} // namespace zong
