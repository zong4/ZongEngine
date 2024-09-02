using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Hazel
{
	public class SceneRenderer
	{

		public static float Opacity
		{
			get
			{
				unsafe { return InternalCalls.SceneRenderer_GetOpacity(); }
			}

			set
			{
				unsafe { InternalCalls.SceneRenderer_SetOpacity(value); }
			}
		}

		public class DepthOfField
		{ 
			public static bool IsEnabled
			{
				get
				{
					unsafe { return InternalCalls.SceneRenderer_DepthOfField_IsEnabled(); }
				}
				
				set
				{
					unsafe { InternalCalls.SceneRenderer_DepthOfField_SetEnabled(value); }
				}
			}

			public static float FocusDistance
			{
				get
				{
					unsafe { return InternalCalls.SceneRenderer_DepthOfField_GetFocusDistance(); }
				}

				set
				{
					unsafe { InternalCalls.SceneRenderer_DepthOfField_SetFocusDistance(value); }
				}
			}

			public static float BlurSize
			{
				get
				{
					unsafe { return InternalCalls.SceneRenderer_DepthOfField_GetBlurSize(); }
				}

				set
				{
					unsafe { InternalCalls.SceneRenderer_DepthOfField_SetBlurSize(value); }
				}
			}


		}


	}
}
