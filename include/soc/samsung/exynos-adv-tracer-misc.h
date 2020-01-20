#ifndef __EXYNOS_ADV_TRACER_MISC_H_
#define __EXYNOS_ADV_TRACER_MISC_H_

#ifdef CONFIG_EXYNOS_ADV_TRACER_MISC
int adv_tracer_misc_plugin_load(const char *name, const char *release_plugin_name);
int adv_tracer_misc_ipc_request_channel(const char *name, void *handler);
#else
int adv_tracer_misc_plugin_load(const char *name, const char *release_plugin_name)
{
	return -1;
}

int adv_tracer_misc_ipc_request_channel(const char *name, void *handler)
{
	return -1;
}
#endif
#endif
