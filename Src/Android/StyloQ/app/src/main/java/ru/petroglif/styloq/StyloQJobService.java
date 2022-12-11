package ru.petroglif.styloq;

import android.app.job.JobInfo;
import android.app.job.JobParameters;
import android.app.job.JobScheduler;
import android.app.job.JobService;
import android.content.ComponentName;
import android.content.Context;

public class StyloQJobService extends JobService {
	private static int JobIdDocStatusPoller = 1007;
	@Override public boolean onStartJob(JobParameters params)
	{
		Context _ctx = getApplication();
		if(_ctx != null && _ctx instanceof StyloQApp) {
			Thread thr = new Thread(new StyloQInterchange.ThreadEngine_SvcPoll((StyloQApp)_ctx));
			thr.start();
			return true;
		}
		else
			return false;
	}
	@Override public boolean onStopJob(JobParameters params)
	{
		return false;
	}
	//
	public static int ScheduleTask(Context ctx) // @construction
	{
		int   success_count = 0;
		ComponentName service_name = new ComponentName(ctx, StyloQJobService.class);
		JobInfo job_info = new JobInfo.Builder(JobIdDocStatusPoller, service_name).
			//setRequiredNetworkType(JobInfo.NETWORK_TYPE_ANY).
			//setRequiresDeviceIdle(false).
			//setRequiresCharging(false).
			setPeriodic(20*60*1000). // Говорят, что менее 15 минут работать не будет!
			build();

		JobScheduler scheduler = (JobScheduler)ctx.getSystemService(/*Context.JOB_SCHEDULER_SERVICE*/JobScheduler.class);
		int result = scheduler.schedule(job_info);
		if(result == JobScheduler.RESULT_SUCCESS) {
			//Log.d(TAG, “Job scheduled successfully!”);
			success_count++;
		}
		return  success_count;
	}
}
