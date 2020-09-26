#pragma once

float map_float(float input, float input_start, float input_end, float output_start, float output_end) {
    return output_start + ((output_end - output_start) / (input_end - input_start)) * (input - input_start);
}

double map_double(double input, double input_start, double input_end, double output_start, double output_end) {
    return output_start + ((output_end - output_start) / (input_end - input_start)) * (input - input_start);
}

int clamp_int(int input, int min, int max) {
    return (input > min) ? ( input < max ? input : max) : min;
}