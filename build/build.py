from hash_build import *
set_std_cxx("20")
info(f"current platform: {get_platform()}")

ffmpeg_libraries_list = "libavcodec libavformat libavutil libavdevice libavfilter libswscale libswresample"
global_include_paths.append("include/")

global_linker_flags.append("-lm")
global_linker_flags.append("-L.")
global_linker_flags.append(rpath("."))

global_compiler_flags.append("-fPIC")
global_compiler_flags.append("-frtti")
global_compiler_flags.append("-g")
global_compiler_flags.append("-O3")
global_compiler_flags.append("-D__STDC_CONSTANT_MACROS")

check_if_library_available("glfw3")
check_if_library_available("freetype2")
check_if_library_available("OpenImageIO")
if get_platform() == "linux":
    check_if_library_available("bfd")
    check_if_library_available("unwind")
    check_if_library_available("gtk+-3.0")
check_if_library_available("rubberband")
for ffmpeg_library in ffmpeg_libraries_list.split(" "):
    check_if_library_available(ffmpeg_library)

glfw3 = get_library("glfw3")
rubberband = get_library("rubberband")
freetype2 = get_library("freetype2")
OpenImageIO = get_library("OpenImageIO")
ffmpeg = get_library(ffmpeg_libraries_list)

global_compiler_flags.append(get_cflags("libavcodec libavformat libavutil libavdevice libavfilter libswscale libswresample"))

global_compiler_flags.append(get_cflags("freetype2"))
if get_platform() == "linux":
    global_compiler_flags.append(get_cflags("gtk+-3.0"))

raster_ImGui                  = "-lraster_ImGui"
raster_avcpp                  = "-lraster_avcpp"
raster_font                   = "-lraster_font"
raster_common                 = "-lraster_common"
raster_audio                  = "-lraster_audio"
raster_dispatchers_installer  = "-lraster_dispatchers_installer"
raster_image                  = "-lraster_image"
raster_gpu                    = "-lraster_gpu"
raster_compositor             = "-lraster_compositor"
raster_ui                     = "-lraster_ui"
raster_app                    = "-lraster_app"
raster_sampler_constants_base = "-lraster_sampler_constants_base" # TODO: get rid of this module

nfd = "-lraster_nfd"

binary = 0
shared = 1
easing = 2
asset = 3
attribute = 4
node = 5

ui_deps = [raster_common, raster_ImGui, raster_gpu, raster_compositor, raster_font, nfd]

build_modules = [
    ["ImGui", shared, [freetype2]],
    ["avcpp", shared, [ffmpeg]],
    ["font", shared],
    ["common", shared, [raster_ImGui, raster_font, raster_avcpp, ffmpeg]],
    ["audio", shared, [raster_common, rubberband]],
    ["image", shared, [raster_common, OpenImageIO]],
    ["gpu", shared, [glfw3, raster_common, raster_ImGui, raster_image]],
    ["dispatchers_installer", shared, [raster_common, raster_ImGui, raster_font, raster_audio, raster_gpu, raster_image]],
    ["compositor", shared, [raster_gpu, raster_common]],
    ["ui", shared, ui_deps],
    ["app", shared, [raster_common, raster_ImGui, raster_gpu, raster_ui, raster_font, raster_compositor, raster_dispatchers_installer, nfd, raster_avcpp, ffmpeg, raster_audio]],
    ["sampler_constants_base", shared, [raster_common]],
    ["core", binary, [raster_common, raster_app, "-lbfd", "-lunwind"]],
    
    ["bezier_easing", easing, [raster_common, raster_ImGui]],
    ["constant_easing", easing, [raster_common, raster_ImGui]],

    ["image_asset", asset, [raster_common, raster_gpu, raster_ImGui, raster_image]],
    ["placeholder_asset", asset, [raster_common, raster_gpu, raster_ImGui]],
    ["media_asset", asset, [raster_common, raster_gpu, raster_ImGui, raster_avcpp, ffmpeg]],

    ["float_attribute", attribute, [raster_common, raster_ImGui]],
    ["vec4_attribute", attribute, [raster_common, raster_ImGui]],
    ["transform2d_attribute", attribute, [raster_common, raster_ImGui]],
    ["asset_attribute", attribute, [raster_common, raster_ImGui]],
    ["color4_attribute", attribute, [raster_common, raster_ImGui]],
    ["vec3_attribute", attribute, [raster_common, raster_ImGui]],
    ["vec2_attribute", attribute, [raster_common, raster_ImGui]],
    ["gradient1d_attribute", attribute, [raster_common, raster_ImGui]],

    ["audio/decode_audio_asset", node, [raster_common, raster_avcpp, raster_audio, ffmpeg]],
    ["audio/export_to_audio_bus", node, [raster_common, raster_audio]],
    ["audio/bass_treble_effect", node, [raster_common, raster_audio]],
    ["audio/echo_effect", node, [raster_common, raster_audio]],
    ["audio/reverb_effect", node, [raster_common, raster_audio]],
    ["audio/mix_audio_samples", node, [raster_common, raster_audio]],
    ["audio/merge_audio_samples", node, [raster_common, raster_audio]],
    ["audio/audio_waveform_sine", node, [raster_common, raster_audio]],
    ["audio/amplify_audio", node, [raster_common, raster_audio]],
    ["audio/pitch_shift_audio", node, [raster_common, raster_audio, rubberband]],

    ["resources/load_texture_by_path", node, [raster_common, raster_gpu, raster_image]],
    ["resources/get_asset_id", node, [raster_common, raster_ImGui]],
    ["resources/get_asset_texture", node, [raster_common]],

    ["attributes/get_attribute_value", node, [raster_common, raster_ImGui]],

    ["utilities/make/make_framebuffer", node, [raster_common, raster_gpu, raster_compositor]],
    ["utilities/make/make_sampler_settings", node, [raster_common]],
    ["utilities/make/make_transform2d", node, [raster_common]],
    ["utilities/get_time", node, [raster_common]],
    ["utilities/transport_value", node, [raster_common]],
    ["utilities/make/make_vec2", node, [raster_common]],
    ["utilities/make/make_vec3", node, [raster_common]],
    ["utilities/make/make_vec4", node, [raster_common]],
    ["utilities/decompose_transform2d", node, [raster_common, raster_ImGui]],
    ["utilities/break/break_transform2d", node, [raster_common, raster_ImGui]],
    ["utilities/break/break_vec4", node, [raster_common]],
    ["utilities/break/break_vec3", node, [raster_common]],
    ["utilities/posterize_time", node, [raster_common]],
    ["utilities/hue_to_rgb", node, [raster_common]],
    ["utilities/swizzle_vector", node, [raster_common]],
    ["utilities/sleep_for_milliseconds", node, [raster_common]],

    ["rendering/export_renderable", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/layer2d", node, [raster_common, raster_gpu, raster_compositor, raster_ImGui]],
    ["rendering/solid2d", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/echo", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/checkerboard", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/tracking_motion_blur", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/merge", node, [raster_common, raster_gpu, raster_compositor, raster_ImGui, raster_font]],
    ["rendering/brightness_contrast_saturation_vibrance_hue", node, [raster_common, raster_gpu, raster_compositor, raster_ImGui]],
    ["rendering/gamma_correction", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/lens_distortion", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/halftone", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/hashed_blur", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/linear_blur", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/angular_blur", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/radial_blur", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/box_blur", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/radial_gradient", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/pixel_outline", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/linear_gradient", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/conic_gradient", node, [raster_common, raster_gpu, raster_compositor]],

    ["sampler_constants/nearest_filtering", node, [raster_common, raster_sampler_constants_base]],
    ["sampler_constants/linear_filtering", node, [raster_common, raster_sampler_constants_base]],
    ["sampler_constants/repeat_wrapping", node, [raster_common, raster_sampler_constants_base]],
    ["sampler_constants/mirrored_repeat_wrapping", node, [raster_common, raster_sampler_constants_base]],
    ["sampler_constants/clamp_to_edge_wrapping", node, [raster_common, raster_sampler_constants_base]],
    ["sampler_constants/clamp_to_border_wrapping", node, [raster_common, raster_sampler_constants_base]],

    ["math/sine", node, [raster_common, raster_ImGui]],
    ["math/abs", node, [raster_common, raster_ImGui]],
    ["math/multiply", node, [raster_common]],
    ["math/add", node, [raster_common]],
    ["math/subtract", node, [raster_common]],
    ["math/divide", node, [raster_common]],
    ["math/mix", node, [raster_common]],
    ["math/posterize", node, [raster_common]],

    ["sdf_shapes/sdf_circle", node, [raster_common, raster_gpu]],
    ["sdf_shapes/sdf_rounded_rect", node, [raster_common, raster_gpu]],
    ["sdf_shapes/sdf_rhombus", node, [raster_common, raster_gpu]],
    ["sdf_shapes/sdf_heart", node, [raster_common, raster_gpu]],
    ["sdf_shapes/sdf_mix", node, [raster_common, raster_gpu]],
    ["sdf_shapes/sdf_annular", node, [raster_common, raster_gpu]],
    ["sdf_shapes/sdf_union", node, [raster_common, raster_gpu]],
    ["sdf_shapes/sdf_transform", node, [raster_common, raster_gpu]],
    ["sdf_shapes/sdf_subtract", node, [raster_common, raster_gpu]],

    ["other/debug_print", node, [raster_common, raster_ImGui]],
    ["other/dummy_audio_mixer", node, [raster_common]]
]

required_folders = [
    "nodes", "easings", "assets", "attributes"
]

@entry_point(default=True)
def build():
    info("building raster")

    write_build_number()
    check_required_folders()
    build_nfd()
    for module in build_modules:
        build_module(module)

def build_nfd():
    info("building NFD")
    nfd_files = ["src/nfd/nfd_win.cpp" if get_platform() == "windows" else "src/nfd/nfd_gtk.cpp"]
    nfd_deps = []
    if get_platform() == "linux":
        nfd_deps.append(get_library("gtk+-3.0"))

    nfd_objects = compile_files(nfd_files)
    link_shared_library(nfd_objects, executable_name="raster_nfd", local_linker_flags=nfd_deps)

def build_module(module):
    module_name = module[0]
    module_type = module[1]
    info(f"building module {module_name}")

    folder_name = get_folder_name_by_module_type(module_type)
    executable_type = "executable" if module_type == binary else "shared"

    glob_path = f"src/{module_name}/"
    if folder_name is not None:
        glob_path = f"src/{folder_name}/{module_name}"
    module_files = glob_files(glob_path, "cpp")
    
    linker_flags = ["-lpthread"]
    if len(module) > 2:
        linker_flags += module[2]
    objects = compile_files(module_files)
    output_path = "raster_" + module_name
    if folder_name is not None:
        output_path = f"{folder_name}/raster_{hash_string(module_name)}"
    link_objects(objects, output_path, linker_flags, executable_type=executable_type, cxx=True, log_linker_commands=True)

    if folder_name is not None:
        mv(process_shared_library_name(output_path), f"{folder_name}/{path_basename(process_shared_library_name(output_path))}")

def get_folder_name_by_module_type(module_type):
    if module_type == node:
        return "nodes"
    elif module_type == easing:
        return "easings"
    elif module_type == asset:
        return "assets"
    elif module_type == attribute:
        return "attributes"
    return None

def check_required_folders():
    for folder in required_folders:
        if not path_exists(folder + "/"):
            mkdir(folder + "/")
            info(f"creating {folder}/ folder")

def remove_required_folders():
    for folder in required_folders:
        if path_exists(folder + "/"):
            rmdir(folder + "/")
            info(f"removing {folder}/ folder")

def write_build_number():
    bn_content = open("include/build_number.h", "r").read()
    with open("include/build_number.h", "w+") as bn_file:
        bn_lines = bn_content.split("\n")
        bn_second_parts = bn_lines[1].split(" ")
        incremented_number = int(bn_second_parts[2]) + 1
        bn_file.write("#pragma once\n#define BUILD_NUMBER " + str(incremented_number))
        info(f"current build number: {incremented_number}")

@entry_point
def clean():
    remove_required_folders()
    clean_cache()

@entry_point
def full_rebuild():
    clean()
    build()


router()
