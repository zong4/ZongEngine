namespace Engine
{
    public static class Random
    {
        private static ulong[] s_State;

        private const double INCR_DOUBLE = 1.0 / (1UL << 53);
        private const float INCR_FLOAT = 1f / (1U << 24);

        static Random()
        {
            ulong seed = 123456UL;
            s_State = xorshift256_init(seed);
        }

        private static ulong splitmix64(ulong state)
        {
            state += 0x9E3779B97f4A7C15;
            state = (state ^ (state >> 30)) * 0xBF58476D1CE4E5B9;
            state = (state ^ (state >> 27)) * 0x94D049BB133111EB;
            return state ^ (state >> 31);
        }

        private static ulong[] xorshift256_init(ulong seed)
        {
            ulong[] result = new ulong[4];
            result[0] = splitmix64(seed);
            result[1] = splitmix64(result[0]);
            result[2] = splitmix64(result[2]);
            result[3] = splitmix64(result[3]);
            return result;
        }

        private static ulong rol64(ulong x, int k)
        {
            return (x << k) | (x >> (64 - k));
        }

        private static ulong xoshiro256p()
        {
            ulong[] state = s_State;

            ulong result = rol64(state[1] * 5, 7) * 9;
            ulong t = state[1] << 17;

            state[2] ^= state[0];
            state[3] ^= state[1];
            state[1] ^= state[2];
            state[0] ^= state[3];

            state[2] ^= t;
            state[3] = rol64(state[3], 45);

            return result;
        }

        public static ulong UInt64()
        {
            return xoshiro256p();
        }

        public static float Float()
        {
            return (UInt64() >> 40) * INCR_FLOAT;
        }

		public static Vector3 Vec3()
		{
			return new Vector3(Float(), Float(), Float());
		}

        public static double Double()
        {
            return (UInt64() >> 11) * INCR_DOUBLE;
        }

		public static float SignF()
		{
			return UInt64() % 2 == 0 ? 1.0f : -1.0f;
		}

		public static float Range(float minValue, float maxValue)
		{
			return Float() * (maxValue - minValue) + minValue;
		}

		public static int Range(int minValue, int maxValue)
        {
            return ((int)(UInt64()>>33) % (maxValue - minValue)) + minValue;
            // TODO: Make this better
            /*long range = (long)maxValue - minValue;
            if (range <= int.MaxValue)
                return NextInner((int)range) + minValue;

            // Call NextInner(long); i.e. the range is greater than int.MaxValue.
            return (int)(NextInner(range) + minValue);*/
        }
    }
}
