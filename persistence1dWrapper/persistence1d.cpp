// Wrapper class for persistence1d code (https://github.com/yeara/Persistence1D)

#include "stdafx.h"
#include "persistence1d.h"

namespace persistence1d
{
	p1d::p1d()
	{
		p = new Persistence1D();
	}

	p1d::~p1d()
	{
		delete p;
	}

	bool p1d::RunPersistence(Collections::Generic::List<double>^ InputData)
	{
		std::vector<double> vec;
		for each (double i in InputData)
		{
			vec.push_back(i);
		}
		return p->RunPersistence(vec);
	}

	bool p1d::GetPairedExtrema(Collections::Generic::List<int>^ mins,
		Collections::Generic::List<int>^ maxs,
		Collections::Generic::List<double>^ persistents)
	{
		return this->GetPairedExtrema(mins, maxs, persistents, 0, false);
	}
	bool p1d::GetPairedExtrema(Collections::Generic::List<int>^ mins,
		Collections::Generic::List<int>^ maxs,
		Collections::Generic::List<double>^ persistents,
		double threshold)
	{
		return this->GetPairedExtrema(mins, maxs, persistents, threshold, false);
	}

	bool p1d::GetPairedExtrema(Collections::Generic::List<int>^ mins,
		Collections::Generic::List<int>^ maxs,
		Collections::Generic::List<double>^ persistents,
		double threshold,
		bool matlabIndexing)
	{
		std::vector<TPairedExtrema> vec;
		bool success = p->GetPairedExtrema(vec, threshold, matlabIndexing);

		if (success == false)
			return false;

		mins->Clear();
		maxs->Clear();
		persistents->Clear();

		for each(TPairedExtrema tp in vec)
		{
			mins->Add(tp.MinIndex);
			maxs->Add(tp.MaxIndex);
			persistents->Add(tp.Persistence);
		}

		return success;
	}


	bool p1d::GetExtremaIndices(
		Collections::Generic::List<int>^ min,
		Collections::Generic::List<int>^ max)
	{
		return this->GetExtremaIndices(min, max, 0, false);
	}

	bool p1d::GetExtremaIndices(
		Collections::Generic::List<int>^ min,
		Collections::Generic::List<int>^ max,
		double threshold)
	{
		return this->GetExtremaIndices(min, max, threshold, false);
	}

	bool p1d::GetExtremaIndices(
		Collections::Generic::List<int>^ min,
		Collections::Generic::List<int>^ max,
		double threshold,
		bool matlabIndexing)
	{
		std::vector<int> minvec;
		std::vector<int> maxvec;
		min->Clear();
		max->Clear();

		bool success = p->GetExtremaIndices(minvec, maxvec, threshold, matlabIndexing);

		if (!success) return false;

		for each (int val in minvec)
		{
			min->Add(val);
		}
		for each (int val in maxvec)
		{
			max->Add(val);
		}

		return true;
	}

	int p1d::GetGlobalMinimumIndex()
	{
		return p->GetGlobalMinimumIndex(false);
	}
	int p1d::GetGlobalMinimumIndex(const bool matlabIndexing)
	{
		return p->GetGlobalMinimumIndex(matlabIndexing);
	}

	double p1d::GetGlobalMinimumValue()
	{
		return p->GetGlobalMinimumValue();
	}

	bool p1d::VerifyResults()
	{
		return p->VerifyResults();
	}
}