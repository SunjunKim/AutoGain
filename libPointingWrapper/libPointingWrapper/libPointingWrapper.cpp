// 기본 DLL 파일입니다.
// Reference:: http://tom-shelton.net/index.php/2008/12/11/creating-a-managed-wrapper-for-a-lib-file/

#include "stdafx.h"
#include "libPointingWrapper.h"

using namespace libPointingWrapper;
using namespace pointing;

TransferFunction *func = 0;
PointingDevice *input = 0;
DisplayDevice *output = 0;

libPointing::libPointing()
{

}

void libPointing::init()
{
	input = PointingDevice::create("any:?debugLevel=1");
	output = DisplayDevice::create("any:?debugLevel=1");
}

void libPointing::setTranslateFunction(const char *fnURI)
{
	if (func != 0)
		delete func;

	func = TransferFunction::create(fnURI, input, output);
}

void libPointing::getTrasnalteValues(int input_dx, int input_dy, int64_t timestamp, double *output_dx, double *output_dy)
{
	func->applyd((double)input_dx, (double)input_dy, output_dx, output_dy, timestamp);
}

void libPointing::clear()
{
	if (input != 0) delete input;
	if (output != 0) delete output;
	if (func != 0)	delete func;
}

libPointing::~libPointing()
{

}
