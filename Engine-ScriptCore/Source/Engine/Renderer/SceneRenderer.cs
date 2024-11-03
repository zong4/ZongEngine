using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Engine
{
	public class SceneRenderer
	{

		public static float Opacity
		{
			get => InternalCalls.SceneRenderer_GetOpacity();
			set => InternalCalls.SceneRenderer_SetOpacity(value);
		}

		public class DepthOfField
		{ 
			public static bool IsEnabled
			{
				get => InternalCalls.SceneRenderer_DepthOfField_IsEnabled();
				set => InternalCalls.SceneRenderer_DepthOfField_SetEnabled(value);
			}

			public static float FocusDistance
			{
				get => InternalCalls.SceneRenderer_DepthOfField_GetFocusDistance();
				set => InternalCalls.SceneRenderer_DepthOfField_SetFocusDistance(value);
			}

			public static float BlurSize
			{
				get => InternalCalls.SceneRenderer_DepthOfField_GetBlurSize();
				set => InternalCalls.SceneRenderer_DepthOfField_SetBlurSize(value);
			}


		}


	}
}
