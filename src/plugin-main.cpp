/*
Plugin Name
Copyright (C) 2023 Tomasz Żyźniewski tomasz.zyzniewski@grandmasters.online

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <graphics/vec4.h>

#include "plugin-macros.generated.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

struct subsampling_detector {
	obs_source_t *const source;

	gs_effect_t *effect = NULL;
	gs_eparam_t *texture_width, *texture_height;
	gs_eparam_t *color_param;

	struct vec4 color;

	subsampling_detector(obs_source_t *source) : source(source) {}
};

static const char *subsampling_get_name(void *data)
{
	UNUSED_PARAMETER(data);

	return "Subsampling Detector";
}

static void subsampling_detector_update(void *data, obs_data_t *settings)
{
	subsampling_detector *ctx = (subsampling_detector *)data;

	uint32_t color = (uint32_t)obs_data_get_int(settings, "color");

	vec4_from_rgba(&ctx->color, color);
}

static void *subsampling_detector_create(obs_data_t *settings,
					 obs_source_t *source)
{
	UNUSED_PARAMETER(settings);

	subsampling_detector *ctx = new subsampling_detector(source);

	char *effect_file = obs_module_file("subsampling-detector.effect");

	obs_enter_graphics();

	ctx->effect = gs_effect_create_from_file(effect_file, NULL);

	if (ctx->effect) {
		ctx->texture_width = gs_effect_get_param_by_name(
			ctx->effect, "texture_width");
		ctx->texture_height = gs_effect_get_param_by_name(
			ctx->effect, "texture_height");
		ctx->color_param =
			gs_effect_get_param_by_name(ctx->effect, "color");
	}

	obs_leave_graphics();

	bfree(effect_file);

	subsampling_detector_update(ctx, settings);

	return ctx;
}

static void subsampling_detector_destroy(void *data)
{
	subsampling_detector *ctx = (subsampling_detector *)data;

	if (ctx->effect) {
		obs_enter_graphics();
		gs_effect_destroy(ctx->effect);
		obs_leave_graphics();
	}

	delete ctx;
}

static void subsampling_video_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);

	subsampling_detector *ctx = (subsampling_detector *)data;

	if (!obs_source_process_filter_begin(ctx->source, GS_RGBA,
					     OBS_ALLOW_DIRECT_RENDERING)) {
		return;
	}

	float width =
		(float)obs_source_get_width(obs_filter_get_target(ctx->source));
	float height = (float)obs_source_get_height(
		obs_filter_get_target(ctx->source));

	gs_effect_set_float(ctx->texture_width, width);
	gs_effect_set_float(ctx->texture_height, height);
	gs_effect_set_vec4(ctx->color_param, &ctx->color);

	obs_source_process_filter_end(ctx->source, ctx->effect, 0, 0);
}

static void subsampling_detector_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "color", 0xFF0000FF);
}

static obs_properties_t *subsampling_detector_get_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *properties = obs_properties_create();

	obs_properties_add_color(properties, "color", obs_module_text("Color"));

	return properties;
}

static struct obs_source_info subsampling_detector_info = {
	.id = "subsampling_detector",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = subsampling_get_name,
	.create = subsampling_detector_create,
	.destroy = subsampling_detector_destroy,
	.get_defaults = subsampling_detector_get_defaults,
	.get_properties = subsampling_detector_get_properties,
	.update = subsampling_detector_update,
	.video_render = subsampling_video_render,
};

bool obs_module_load(void)
{
	obs_register_source(&subsampling_detector_info);

	return true;
}

void obs_module_unload() {}
