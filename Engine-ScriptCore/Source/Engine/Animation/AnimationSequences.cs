using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Engine
{
	public class AnimationSequenceTransform : AnimationSequenceGeneric<Transform>
	{
		private static Dictionary<KeyframeType, Func<Vector3, Vector3, float, Vector3>> m_InterpolateFuncs = new Dictionary<KeyframeType, Func<Vector3, Vector3, float, Vector3>>()
		{
			{ KeyframeType.Linear, (Vector3 a, Vector3 b, float t) => Engine.Interpolate.Linear(a, b, t) },
			{ KeyframeType.EaseIn, (Vector3 a, Vector3 b, float t) => Engine.Interpolate.EaseIn(a, b, t) },
			{ KeyframeType.EaseOut, (Vector3 a, Vector3 b, float t) => Engine.Interpolate.EaseOut(a, b, t) },
			{ KeyframeType.EaseInOut, (Vector3 a, Vector3 b,float t) => Engine.Interpolate.EaseInOut(a, b, t) },
			{ KeyframeType.Hold, (Vector3 a, Vector3 b, float t) => a }
		};

		protected override Transform Interpolate(KeyframeType type, Transform a, Transform b, float time)
		{
			Vector3 translation = m_InterpolateFuncs[type](a.Position, b.Position, time);
			Vector3 rotation = m_InterpolateFuncs[type](a.Rotation, b.Rotation, time);
			Vector3 scale = m_InterpolateFuncs[type](a.Scale, b.Scale, time);

			return new Transform(translation, rotation, scale);
		}

	}

	public class AnimationSequenceFloat : AnimationSequenceGeneric<float>
	{
		private static Dictionary<KeyframeType, Func<float, float, float, float>> m_InterpolateFuncs = new Dictionary<KeyframeType, Func<float, float, float, float>>()
		{
			{ KeyframeType.Linear, (float a, float b, float t) => Engine.Interpolate.Linear(a, b, t) },
			{ KeyframeType.EaseIn, (float a, float b, float t) => Engine.Interpolate.EaseIn(a, b, t) },
			{ KeyframeType.EaseOut, (float a, float b, float t) => Engine.Interpolate.EaseOut(a, b, t) },
			{ KeyframeType.EaseInOut, (float a, float b,float t) => Engine.Interpolate.EaseInOut(a, b, t) },
			{ KeyframeType.Hold, (float a, float b, float t) => a }
		};

		protected override float Interpolate(KeyframeType type, float a, float b, float time) => m_InterpolateFuncs[type](a, b, time);
	}


	public class AnimationSequenceVector2 : AnimationSequenceGeneric<Vector2>
	{
		private static Dictionary<KeyframeType, Func<Vector2, Vector2, float, Vector2>> m_InterpolateFuncs = new Dictionary<KeyframeType, Func<Vector2, Vector2, float, Vector2>>()
		{
			{ KeyframeType.Linear, (Vector2 a, Vector2 b, float t) => Engine.Interpolate.Linear(a, b, t) },
			{ KeyframeType.EaseIn, (Vector2 a, Vector2 b, float t) => Engine.Interpolate.EaseIn(a, b, t) },
			{ KeyframeType.EaseOut, (Vector2 a, Vector2 b, float t) => Engine.Interpolate.EaseOut(a, b, t) },
			{ KeyframeType.EaseInOut, (Vector2 a, Vector2 b,float t) => Engine.Interpolate.EaseInOut(a, b, t) },
			{ KeyframeType.Hold, (Vector2 a, Vector2 b, float t) => a }
		};

		protected override Vector2 Interpolate(KeyframeType type, Vector2 a, Vector2 b, float time) => m_InterpolateFuncs[type](a, b, time);
	}

	public class AnimationSequenceVector3 : AnimationSequenceGeneric<Vector3>
	{
		private static Dictionary<KeyframeType, Func<Vector3, Vector3, float, Vector3>> m_InterpolateFuncs = new Dictionary<KeyframeType, Func<Vector3, Vector3, float, Vector3>>()
		{
			{ KeyframeType.Linear, (Vector3 a, Vector3 b, float t) => Engine.Interpolate.Linear(a, b, t) },
			{ KeyframeType.EaseIn, (Vector3 a, Vector3 b, float t) => Engine.Interpolate.EaseIn(a, b, t) },
			{ KeyframeType.EaseOut, (Vector3 a, Vector3 b, float t) => Engine.Interpolate.EaseOut(a, b, t) },
			{ KeyframeType.EaseInOut, (Vector3 a, Vector3 b,float t) => Engine.Interpolate.EaseInOut(a, b, t) },
			{ KeyframeType.Hold, (Vector3 a, Vector3 b, float t) => a }
		};

		protected override Vector3 Interpolate(KeyframeType type, Vector3 a, Vector3 b, float time) => m_InterpolateFuncs[type](a, b, time);
	}
}
