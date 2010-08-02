/*
** Copyright (C) 1998-2010 George Tzanetakis <gtzan@cs.uvic.ca>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "BeatPhase.h"

using std::ostringstream;
using std::cout;
using std::endl;

using namespace Marsyas;

BeatPhase::BeatPhase(mrs_string name):MarSystem("BeatPhase", name)
{
	addControls();
	sampleCount_ = 0;
}

BeatPhase::BeatPhase(const BeatPhase& a) : MarSystem(a)
{
	ctrl_tempos_ = getctrl("mrs_realvec/tempos");
	ctrl_temposcores_ = getctrl("mrs_realvec/tempo_scores");
	ctrl_phase_tempo_ = getctrl("mrs_real/phase_tempo");
	
	ctrl_beats_ = getctrl("mrs_realvec/beats");
	ctrl_bhopSize_ = getctrl("mrs_natural/bhopSize");
	ctrl_bwinSize_ = getctrl("mrs_natural/bwinSize");
	ctrl_timeDomain_ = getctrl("mrs_realvec/timeDomain");

	ctrl_beatOutput_ = getctrl("mrs_realvec/beatOutput");
	
	sampleCount_ = 0;
}

BeatPhase::~BeatPhase()
{
}

MarSystem* 
BeatPhase::clone() const 
{
	return new BeatPhase(*this);
}

void 
BeatPhase::addControls()
{
	//Add specific controls needed by this MarSystem.
	addctrl("mrs_realvec/tempos", realvec(2), ctrl_tempos_);
	addctrl("mrs_realvec/tempo_scores", realvec(2), ctrl_temposcores_);
	
	addctrl("mrs_real/phase_tempo", 100.0, ctrl_phase_tempo_);
	addctrl("mrs_realvec/beats", realvec(), ctrl_beats_);
	addctrl("mrs_natural/bhopSize", 64, ctrl_bhopSize_);
	addctrl("mrs_natural/bwinSize", 1024, ctrl_bwinSize_);
	addctrl("mrs_realvec/timeDomain", realvec(), ctrl_timeDomain_);

	addctrl("mrs_realvec/beatOutput", realvec(), ctrl_beatOutput_);
}

void
BeatPhase::myUpdate(MarControlPtr sender)
{
	// no need to do anything BeatPhase-specific in myUpdate 
	MarSystem::myUpdate(sender);

	inSamples_ = getctrl("mrs_natural/inSamples")->to<mrs_natural>();

	if (pinSamples_ != inSamples_)
	{
		{
			MarControlAccessor acc(ctrl_beats_);
			mrs_realvec& beats = acc.to<mrs_realvec>();
			beats.create(inSamples_);

            // Output all the beats that are detected via a MarControl
			MarControlAccessor beatOutputAcc(ctrl_beatOutput_);
			mrs_realvec& beatOutput = beatOutputAcc.to<mrs_realvec>();
			beatOutput.create(inSamples_);
		}
	}
   
	pinSamples_ = inSamples_;


   
}


void 
BeatPhase::myProcess(realvec& in, realvec& out)
{
	mrs_natural o,t;
	
	MarControlAccessor acct(ctrl_tempos_);
	mrs_realvec& tempos = acct.to<mrs_realvec>();

	MarControlAccessor accts(ctrl_temposcores_);
	// mrs_realvec& tempo_scores = accts.to<mrs_realvec>();
	
	
	mrs_natural bwinSize = ctrl_bwinSize_->to<mrs_natural>();
	mrs_natural bhopSize = ctrl_bhopSize_->to<mrs_natural>();

	MarControlAccessor acc(ctrl_beats_);
	mrs_realvec& beats = acc.to<mrs_realvec>();

	mrs_real period;
	mrs_real sum_phase = 0.0;
	mrs_real max_sum_phase = 0.0;
	mrs_natural max_phase = 0;
	// mrs_real max_phase_tempo;
	mrs_real avg_period; 
	// mrs_real next_peak;
	
	
	beats.setval(0.0);

	for (o=0; o < inObservations_; o++)
	{
		for (t = 0; t < inSamples_; t++)
		{
			out (o,t) = in(o,t);
			beats (o,t) = in(o,t);
		}
	}
	
	
	mrs_real tempo;
	// mrs_real max_tm;
	
	max_sum_phase = 0;
	
	ctrl_phase_tempo_->setValue(tempos(0));	
		
	tempo = tempos(0);

	if (tempo !=0)
		period = 2.0 * osrate_ * 60.0 / tempo; // flux hopSize is half the winSize 
	else 
		period = 0;
	period = (mrs_natural)(period+0.5);	

	
	max_sum_phase = 0.0;
	
	for (t= inSamples_-1; t >= inSamples_-1-period; t--) 
	{
		sum_phase = 0.0;
		sum_phase += in(0,t);
		sum_phase += in(0, t-(mrs_natural)period);
		sum_phase += in(0, t-(mrs_natural)(2*period));
		sum_phase += in(0, t-(mrs_natural)(3*period));
		if (sum_phase >= max_sum_phase)
		{
			max_sum_phase = sum_phase;
			max_phase = t;
		}
	}
	

	beats(0, max_phase) = -0.5;
	beats(0, max_phase - period) = -0.5;
	beats(0, max_phase - 2*period) = -0.5;
	beats(0, max_phase - 3*period) = -0.5;
	
	
	
	mrs_natural delay = (bwinSize - bhopSize);
	mrs_natural start;
	mrs_natural end;
	mrs_natural prev_phase = max_phase;
	mrs_natural prev_sample_count;
	
	/* avg_period = 0;
	
	for (int k=0; k < 4; k++)
	{
			start = max_phase - (mrs_natural)period - 2;
			end  =  max_phase - (mrs_natural)period + 2;
			max_sum_phase = 0.0;
			for (t=start; t <= end; t++)
		{
			sum_phase = in(0,t);
			if (sum_phase >= max_sum_phase) 
			{
				max_sum_phase = sum_phase;
				max_phase = t;
			}
		}

		avg_period += (prev_phase - max_phase);
		prev_phase = max_phase;

	}
	
	avg_period /= 4;
	*/ 

	prev_sample_count = sampleCount_;

    // Output all the detected beats to it's own MarControl
    int total_beats = 0;
	MarControlAccessor beatOutputAcc(ctrl_beatOutput_);
	mrs_realvec& beatOutput = beatOutputAcc.to<mrs_realvec>();
    for (t = 0; t < inSamples_; t++)
    {
      beatOutput(t) = 0.0;
    }

 	for (int t = inSamples_-1; t >= inSamples_-1-bhopSize; t--) 
	{
		if (beats(0, t) == -0.5)
		{
			// cout << (sampleCount_  - delay) / (2.0 * osrate_) * 1000.0 << endl;
			// Audacity label output
			// cout << (sampleCount_  - delay) / (2.0 * osrate_) << "\t";
			// cout << (sampleCount_-1 - delay) / (2.0 * osrate_) << " b" << endl;
			/* cout << "SampleCount = " << sampleCount_ << endl;
			cout << "t = " << t << endl;
			cout << "inSamples_ = " << inSamples_ << endl;
			cout << "bhopSize = " << bhopSize << endl;
			
			cout << "nt = " << t - (inSamples_-1-bhopSize) << endl;
			*/ 
			// cout <<  (sampleCount_ + t - (inSamples_-1-bhopSize)) / (2.0 * osrate_) << "\t";
			// cout << (sampleCount_ + t - (inSamples_-1-bhopSize+1)) / (2.0 * osrate_) << " b" << endl;
			beats(0,t) = -1.0;

            // Add that beat to the beatOutput control
            beatOutput(total_beats) = (sampleCount_ + t - (inSamples_-1-bhopSize)) / (2.0 * osrate_);
            total_beats++;
		}
	}
	sampleCount_ += bhopSize;
	

	/* sampleCount_ += bhopSize;
	if (sampleCount_ == bwinSize) 
	{
		cout << "FULL WINDOW" << endl;
		t = max_phase;
		for (t = max_phase; t > 0; t-=period)
		{
			beats(0,t) = -0.5;
		}
		
	}
	*/ 
}







	
