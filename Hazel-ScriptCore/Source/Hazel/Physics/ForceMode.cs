
namespace Hazel
{
	public enum EForceMode
	{
		Force = 0,			// A standard force, using Force = mass * distance / time^2
		Impulse,			// An Impulse force, using Force = mass * distance / time
		VelocityChange,		// An Impulse that ignores the objects mass, e.g Force = distance / time
		Acceleration		// A constant force, not accounting for mass, e.g Force = distance / time^2
	}
}
