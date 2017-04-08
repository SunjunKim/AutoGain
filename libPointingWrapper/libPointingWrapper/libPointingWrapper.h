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
		// TODO: ���⿡ �� Ŭ������ ���� �޼��带 �߰��մϴ�.
	};
}
