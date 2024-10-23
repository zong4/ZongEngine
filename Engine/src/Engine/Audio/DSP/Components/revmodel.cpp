#include <pch.h>

// Reverb model implementation
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain

#include "revmodel.hpp"

revmodel::revmodel(double sampleRate)
{
	// the tuning values are set for 44100Hz sample rate, need to adjust accordingly
	const double srCoef = 44100.0 / sampleRate;

	// TODO: this is quite wordy
	bufcombL1.resize((int)(combtuningL1 / srCoef));
	bufcombR1.resize((int)(combtuningR1 / srCoef));
	bufcombL2.resize((int)(combtuningL2 / srCoef));
	bufcombR2.resize((int)(combtuningR2 / srCoef));
	bufcombL3.resize((int)(combtuningL3 / srCoef));
	bufcombR3.resize((int)(combtuningR3 / srCoef));
	bufcombL4.resize((int)(combtuningL4 / srCoef));
	bufcombR4.resize((int)(combtuningR4 / srCoef));
	bufcombL5.resize((int)(combtuningL5 / srCoef));
	bufcombR5.resize((int)(combtuningR5 / srCoef));
	bufcombL6.resize((int)(combtuningL6 / srCoef));
	bufcombR6.resize((int)(combtuningR6 / srCoef));
	bufcombL7.resize((int)(combtuningL7 / srCoef));
	bufcombR7.resize((int)(combtuningR7 / srCoef));
	bufcombL8.resize((int)(combtuningL8 / srCoef));
	bufcombR8.resize((int)(combtuningR8 / srCoef));
	bufallpassL1.resize((int)(allpasstuningL1 / srCoef));
	bufallpassR1.resize((int)(allpasstuningR1 / srCoef));
	bufallpassL2.resize((int)(allpasstuningL2 / srCoef));
	bufallpassR2.resize((int)(allpasstuningR2 / srCoef));
	bufallpassL3.resize((int)(allpasstuningL3 / srCoef));
	bufallpassR3.resize((int)(allpasstuningR3 / srCoef));
	bufallpassL4.resize((int)(allpasstuningL4 / srCoef));
	bufallpassR4.resize((int)(allpasstuningR4 / srCoef));

	// Tie the components to their buffers
	combL[0].setbuffer(bufcombL1.data(), (int)bufcombL1.size());
	combR[0].setbuffer(bufcombR1.data(), (int)bufcombR1.size());
	combL[1].setbuffer(bufcombL2.data(), (int)bufcombL2.size());
	combR[1].setbuffer(bufcombR2.data(), (int)bufcombR2.size());
	combL[2].setbuffer(bufcombL3.data(), (int)bufcombL3.size());
	combR[2].setbuffer(bufcombR3.data(), (int)bufcombR3.size());
	combL[3].setbuffer(bufcombL4.data(), (int)bufcombL4.size());
	combR[3].setbuffer(bufcombR4.data(), (int)bufcombR4.size());
	combL[4].setbuffer(bufcombL5.data(), (int)bufcombL5.size());
	combR[4].setbuffer(bufcombR5.data(), (int)bufcombR5.size());
	combL[5].setbuffer(bufcombL6.data(), (int)bufcombL6.size());
	combR[5].setbuffer(bufcombR6.data(), (int)bufcombR6.size());
	combL[6].setbuffer(bufcombL7.data(), (int)bufcombL7.size());
	combR[6].setbuffer(bufcombR7.data(), (int)bufcombR7.size());
	combL[7].setbuffer(bufcombL8.data(), (int)bufcombL8.size());
	combR[7].setbuffer(bufcombR8.data(), (int)bufcombR8.size());
	allpassL[0].setbuffer(bufallpassL1.data(), (int)bufallpassL1.size());
	allpassR[0].setbuffer(bufallpassR1.data(), (int)bufallpassR1.size());
	allpassL[1].setbuffer(bufallpassL2.data(), (int)bufallpassL2.size());
	allpassR[1].setbuffer(bufallpassR2.data(), (int)bufallpassR2.size());
	allpassL[2].setbuffer(bufallpassL3.data(), (int)bufallpassL3.size());
	allpassR[2].setbuffer(bufallpassR3.data(), (int)bufallpassR3.size());
	allpassL[3].setbuffer(bufallpassL4.data(), (int)bufallpassL4.size());
	allpassR[3].setbuffer(bufallpassR4.data(), (int)bufallpassR4.size());

	// Set default values
	allpassL[0].setfeedback(0.5f);
	allpassR[0].setfeedback(0.5f);
	allpassL[1].setfeedback(0.5f);
	allpassR[1].setfeedback(0.5f);
	allpassL[2].setfeedback(0.5f);
	allpassR[2].setfeedback(0.5f);
	allpassL[3].setfeedback(0.5f);
	allpassR[3].setfeedback(0.5f);
	setwet(initialwet);
	setroomsize(initialroom);
	setdry(initialdry);
	setdamp(initialdamp);
	setwidth(initialwidth);
	setmode(initialmode);

	// Buffer will be full of rubbish - so we MUST mute them
	mute();
}

void revmodel::mute()
{
	if (getmode() >= freezemode)
		return;

	for (int i=0;i<numcombs;i++)
	{
		combL[i].mute();
		combR[i].mute();
	}
	for (int i=0;i<numallpasses;i++)
	{
		allpassL[i].mute();
		allpassR[i].mute();
	}
}

void revmodel::processreplace(const float *inputL, const float *inputR, float *outputL, float *outputR, long numsamples, int skip)
{
	float outL,outR,input;

	while(numsamples-- > 0)
	{
		outL = outR = 0;
		input = (*inputL + *inputR) * gain;

		// Accumulate comb filters in parallel
		for(int i=0; i<numcombs; i++)
		{
			outL += combL[i].process(input);
			outR += combR[i].process(input);
		}

		// Feed through allpasses in series
		for(int i=0; i<numallpasses; i++)
		{
			outL = allpassL[i].process(outL);
			outR = allpassR[i].process(outR);
		}

		// Calculate output REPLACING anything already there
		*outputL = outL*wet1 + outR*wet2 + *inputL*dry;
		*outputR = outR*wet1 + outL*wet2 + *inputR*dry;

		// Increment sample pointers, allowing for interleave (if any)
		inputL += skip;
		inputR += skip;
		outputL += skip;
		outputR += skip;
	}
}

void revmodel::processmix(const float *inputL, const float *inputR, float *outputL, float *outputR, long numsamples, int skip)
{
	float outL,outR,input;

	while(numsamples-- > 0)
	{
		outL = outR = 0;
		input = (*inputL + *inputR) * gain;

		// Accumulate comb filters in parallel
		for(int i=0; i<numcombs; i++)
		{
			outL += combL[i].process(input);
			outR += combR[i].process(input);
		}

		// Feed through allpasses in series
		for(int i=0; i<numallpasses; i++)
		{
			outL = allpassL[i].process(outL);
			outR = allpassR[i].process(outR);
		}

		// Calculate output MIXING with anything already there
		*outputL += outL*wet1 + outR*wet2 + *inputL*dry;
		*outputR += outR*wet1 + outL*wet2 + *inputR*dry;

		// Increment sample pointers, allowing for interleave (if any)
		inputL += skip;
		inputR += skip;
		outputL += skip;
		outputR += skip;
	}
}

void revmodel::update()
{
// Recalculate internal values after parameter change

	int i;

	wet1 = wet*(width/2 + 0.5f);
	wet2 = wet*((1-width)/2);

	if (mode >= freezemode)
	{
		roomsize1 = 1;
		damp1 = 0;
		gain = muted;
	}
	else
	{
		roomsize1 = roomsize;
		damp1 = damp;
		gain = fixedgain;
	}

	for(i=0; i<numcombs; i++)
	{
		combL[i].setfeedback(roomsize1);
		combR[i].setfeedback(roomsize1);
	}

	for(i=0; i<numcombs; i++)
	{
		combL[i].setdamp(damp1);
		combR[i].setdamp(damp1);
	}
}

// The following get/set functions are not inlined, because
// speed is never an issue when calling them, and also
// because as you develop the reverb model, you may
// wish to take dynamic action when they are called.

void revmodel::setroomsize(float value)
{
	roomsize = (value*scaleroom) + offsetroom;
	update();
}

float revmodel::getroomsize()
{
	return (roomsize-offsetroom)/scaleroom;
}

void revmodel::setdamp(float value)
{
	damp = value*scaledamp;
	update();
}

float revmodel::getdamp()
{
	return damp/scaledamp;
}

void revmodel::setwet(float value)
{
	wet = value*scalewet;
	update();
}

float revmodel::getwet()
{
	return wet/scalewet;
}

void revmodel::setdry(float value)
{
	dry = value*scaledry;
}

float revmodel::getdry()
{
	return dry/scaledry;
}

void revmodel::setwidth(float value)
{
	width = value;
	update();
}

float revmodel::getwidth()
{
	return width;
}

void revmodel::setmode(float value)
{
	mode = value;
	update();
}

float revmodel::getmode()
{
	if (mode >= freezemode)
		return 1;
	else
		return 0;
}

//ends
