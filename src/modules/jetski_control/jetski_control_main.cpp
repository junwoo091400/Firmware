/****************************************************************************
 *
 *   Copyright (c) 2020 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/*
Program Info:
- Written by Junwoo Hwang
- Used for controlling Jetski Waterjet using ADC throttle input

Referenced the following Files for structure & coding styles:
"src\examples\fake_magnetometer\FakeMagnetometer.cpp"
"src\modules\fw_att_control\FixedwingAttitudeControl.cpp"
"src\drivers\adc\ADC.cpp" for custom_commands() function



*/

#include "jetski_control_main.hpp"

using namespace time_literals;

JetSkiControl::JetSkiControl() :
	ModuleParams(nullptr),
	ScheduledWorkItem(MODULE_NAME, px4::wq_configurations::hp_default)
{

}

bool JetSkiControl::init()
{
	ScheduleOnInterval(10_ms); // 100 Hz
	return true;
}

void JetSkiControl::Run()
{
	adc_report adc;
	int32_t adc_id = _param_js_adc_id.get();
	float output_val;

	if (should_exit()) {
		ScheduleClear();
		exit_and_cleanup();
		return;
	}

	if (_adc_report_sub.update(&adc)) {
		// If we haven't found the index of the ADC input yet, find it.
		if (_throttle_adc_idx == -1) {
			for (unsigned i = 0; i < PX4_MAX_ADC_CHANNELS; ++i) {
				if (adc.channel_id[i] >= 0 && adc.channel_id[i] == adc_id) {
					_throttle_adc_idx = i;
					continue;
				}
			}
		}
		output_val = calculate_jet_output_from_adc(adc.raw_data[_throttle_adc_idx]);
	}
}

int JetSkiControl::task_spawn(int argc, char *argv[])
{
	JetSkiControl *instance = new JetSkiControl();

	if (instance) {
		_object.store(instance);
		_task_id = task_id_is_work_queue;

		if (instance->init()) {
			return PX4_OK;
		}

	} else {
		PX4_ERR("alloc failed");
	}

	delete instance;
	_object.store(nullptr);
	_task_id = -1;

	return PX4_ERROR;
}

float JetSkiControl::calculate_jet_output_from_adc(int32_t adc_val) {
	float raw_output
	return math::constrain(raw_output, 0.0f, 1.0f);
}

int JetSkiControl::test() {
	uORB::Subscription	adc_sub_test{ORB_ID(adc_report)};
	adc_report_s adc;

	PX4_INFO_RAW("JetSkiControl test starting.\n");
	PX4_INFO_RAW("Current ADC ID: %d, ADC idx: %d\n", _param_js_adc_id.get(), _throttle_adc_idx)
	px4_usleep(20000);	// sleep 20ms and wait for adc report

	if (adc_sub_test.update(&adc)) {

		for (unsigned l = 0; l < 20; ++l) {
			float output_val = calculate_jet_output_from_adc(adc.raw_data[_throttle_adc_idx]);
			PX4_INFO_RAW("ID(% 3d)\tVAL: % 6d\tOut:% f", adc.channel_id[_throttle_adc_idx], adc.raw_data[_throttle_adc_idx], output_val);
			px4_usleep(500000);

			if (!adc_sub_test.update(&adc)) {
				PX4_INFO_RAW("\t ADC sample not updated!\n");
			}
			PX4_INFO_RAW(" DeviceID: %d", adc.device_id);
			PX4_INFO_RAW(" Resolution: %d", adc.resolution);
			PX4_INFO_RAW(" Voltage Reference: %f\n", (double)adc.v_ref);
		}

		PX4_INFO_RAW("\t ADC test successful.\n");

		return 0;

	} else {
		return 1;
	}
}

int JetSkiControl::calibrate() {
	float adc_low, adc_high;
	bool adc_input_reversed = false;
}

int JetSkiControl::custom_command(int argc, char *argv[])
{
	const char *verb = argv[0];

	if (!strcmp(verb, "test")) {
		if (is_running()) {
			return _object.load()->test();
		}

		return PX4_ERROR;
	}
	else if(!strcmp(verb, "calibrate")) {
		if (is_running()) {
			return _object.load()->calibrate();
		}

		return PX4_ERROR;
	}

	return print_usage("unknown command");
}

int JetSkiControl::print_usage(const char *reason)
{
	if (reason) {
		PX4_WARN("%s\n", reason);
	}

	PRINT_MODULE_DESCRIPTION(
		R"DESCR_STR(
### Description
Publish the earth magnetic field as a fake magnetometer (sensor_mag).
Requires vehicle_attitude and vehicle_gps_position.
)DESCR_STR");

	PRINT_MODULE_USAGE_NAME("fake_magnetometer", "driver");
	PRINT_MODULE_USAGE_COMMAND("start");
	PRINT_MODULE_USAGE_DEFAULT_COMMANDS();
	return 0;
}

extern "C" __EXPORT int jetski_control_main(int argc, char *argv[])
{
	return JetSkiControl::main(argc, argv);
}
