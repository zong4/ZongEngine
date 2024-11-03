using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Engine
{
	public enum KeyframeType
	{
		Linear = 0, EaseIn, EaseOut, EaseInOut, Hold
	}

	public struct Keyframe<T> where T : new()
	{ 
		public T Data;
		public float Time;
		public KeyframeType Type;
	}

	public abstract class AnimationSequenceGeneric<T> where T : new()
	{

		public List<Keyframe<T>> Keyframes = new List<Keyframe<T>>();

		public void AddKeyframe(T data, float time, KeyframeType type = KeyframeType.Linear)
		{
			Keyframes.Add(new Keyframe<T>(){Data = data, Time = time, Type = type});
		}

		protected abstract T Interpolate(KeyframeType type, T a, T b, float time);

		public T GetValue(float time)
		{
			for (int i = 0; i < Keyframes.Count; i++)
			{ 
				Keyframe<T> keyframe = Keyframes[i];
				if (time >= keyframe.Time)
				{
					if (Keyframes.Count > i + 1)
					{
						Keyframe<T> nextKeyframe = Keyframes[i + 1];
						if (time <= nextKeyframe.Time)
						{
							// interpolate
							float t = (time - keyframe.Time) / (nextKeyframe.Time - keyframe.Time);
							T data = Interpolate(keyframe.Type, keyframe.Data, nextKeyframe.Data, t);
							return data;
						}
						else
						{
							continue;
						}
					}
					else
					{
						return keyframe.Data;
					}
				}
			}

			if (Keyframes.Count > 0)
				return Keyframes[0].Data;

			return new T();
		}




	}
}
