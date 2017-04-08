// libPointingWrapper.h

#pragma once
using namespace System;

namespace libPointingWrapper {

public ref class libPointing
	{
	public:
		libPointing();
		void init(void);
		void setTranslateFunction(const char * fnURI);
		void getTrasnalteValues(int input_dx, int input_dy, int64_t timestamp, double *output_dx, double *output_dy);
		void clear(void);
		~libPointing();
		// TODO: 여기에 이 클래스에 대한 메서드를 추가합니다.
	};
}
