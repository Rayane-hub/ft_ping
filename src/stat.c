#include "ping.h"

void compute_stat(PingData *data, double deltaT)
{
	if (data->rtt_min > deltaT)
		data->rtt_min = deltaT;
	if (data->rtt_max < deltaT)
		data->rtt_max = deltaT;
	data->rtt_sum += deltaT;
	data->rtt_sum_sq += deltaT*deltaT;
}

void print_stat(PingData *data)
{
	double avg = 0;
	double variance = 0;
	double stddev = 0;
	if (data->received == 0)
	{
		data->rtt_min = 0;
		data->rtt_max = 0;
	}
	else
	{
		avg = data->rtt_sum / data->received;
		variance = (data->rtt_sum_sq / data->received) - (avg * avg);
		if (variance < 0)
			variance = 0;
		stddev = sqrt(variance);
	}
	printf("--- %s ping statistics ---\n", data->host);
	printf("%d packets transmitted, %d packets received, %d%% packet loss\n", data->transmitted, data->received, data->lost);	
	printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n", data->rtt_min, avg, data->rtt_max, stddev);
}
