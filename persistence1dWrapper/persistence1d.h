// persistence1dWrapper.h

#pragma once

using namespace System;
using namespace p1d;

namespace persistence1d {
	public ref class p1d
	{
		// TODO: 여기에 이 클래스에 대한 메서드를 추가합니다.
	protected:
		Persistence1D* p;

	public:
		p1d();
		virtual ~p1d();


		bool RunPersistence(Collections::Generic::List<float>^ InputData);
		
		bool GetPairedExtrema(
			Collections::Generic::List<int>^ mins,
			Collections::Generic::List<int>^ maxs,
			Collections::Generic::List<float>^ persistents);
		bool GetPairedExtrema(
			Collections::Generic::List<int>^ mins, 
			Collections::Generic::List<int>^ maxs, 
			Collections::Generic::List<float>^ persistents,
			float threshold	);
		bool GetPairedExtrema(
			Collections::Generic::List<int>^ mins,
			Collections::Generic::List<int>^ maxs,
			Collections::Generic::List<float>^ persistents, 
			float threshold, 
			bool matlabIndexing);
		

		bool GetExtremaIndices(
			Collections::Generic::List<int>^ min,
			Collections::Generic::List<int>^ max); 

		bool GetExtremaIndices(
			Collections::Generic::List<int>^ min,
			Collections::Generic::List<int>^ max,
			float threshold);

		bool GetExtremaIndices(
			Collections::Generic::List<int>^ min,
			Collections::Generic::List<int>^ max,
			float threshold,
			bool matlabIndexing);

		int GetGlobalMinimumIndex();
		int GetGlobalMinimumIndex(const bool matlabIndexing);

		float GetGlobalMinimumValue();

		bool VerifyResults();
	};
}
