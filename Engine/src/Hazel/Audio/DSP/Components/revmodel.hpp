// Reverb model declaration
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain

#include "Hazel/Audio/SourceManager.h"

#ifndef _revmodel_
#define _revmodel_

#include "comb.hpp"
#include "allpass.hpp"
#include "tuning.h"

class revmodel
{
public:
					revmodel(double sampleRate);
			void	mute();
			void	processmix(const float *inputL, const float *inputR, float *outputL, float *outputR, long numsamples, int skip);
			void	processreplace(const float *inputL, const float *inputR, float *outputL, float *outputR, long numsamples, int skip);
			void	setroomsize(float value);
			float	getroomsize();
			void	setdamp(float value);
			float	getdamp();
			void	setwet(float value);
			float	getwet();
			void	setdry(float value);
			float	getdry();
			void	setwidth(float value);
			float	getwidth();
			void	setmode(float value);
			float	getmode();
private:
			void	update();
private:
	float	gain;
	float	roomsize,roomsize1;
	float	damp,damp1;
	float	wet,wet1,wet2;
	float	dry;
	float	width;
	float	mode;

	// The following are all declared inline 
	// to remove the need for dynamic allocation
	// with its subsequent error-checking messiness

	// Comb filters
	comb	combL[numcombs];
	comb	combR[numcombs];

	// Allpass filters
	allpass	allpassL[numallpasses];
	allpass	allpassR[numallpasses];

	// Buffers for the combs
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufcombL1{ 1116 };
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufcombR1{ 1116 + stereospread };
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufcombL2{ 1188 };
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufcombR2{ 1188 + stereospread };
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufcombL3{ 1277 };
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufcombR3{ 1277 + stereospread };
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufcombL4{ 1356 };
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufcombR4{ 1356 + stereospread };
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufcombL5{ 1422 };
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufcombR5{ 1422 + stereospread };
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufcombL6{ 1491 };
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufcombR6{ 1491 + stereospread };
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufcombL7{ 1557 };
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufcombR7{ 1557 + stereospread };
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufcombL8{ 1617 };
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufcombR8{ 1617 + stereospread };
	
	// Buffers for the allpasses															  
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufallpassL1{ 556 };
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufallpassR1{ 556 + stereospread };
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufallpassL2{ 441 };
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufallpassR2{ 441 + stereospread };
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufallpassL3{ 341 };
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufallpassR3{ 341 + stereospread };
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufallpassL4{ 225 };
	std::vector<float, Hazel::SourceManager::Allocator<float>>	bufallpassR4{ 225 + stereospread };
};

#endif//_revmodel_

//ends