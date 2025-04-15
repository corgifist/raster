from hash_build import *
set_std_cxx("17")
current_platform = get_platform()
info(f"current platform: {current_platform}")

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

build_dependencies = [
    "glfw3", "freetype2", "OpenImageIO", "rubberband",
    "libavcodec", "libavformat", "libavutil", "libavdevice",
    "libavfilter", "libswscale", "libswresample",
    "OpenColorIO"
]

check_if_header_exists("glm/glm.hpp")

if get_platform() == "linux":
    build_dependencies += [
        "bfd", "unwind", "gtk+-3.0"
    ]

failed_dependencies = []
for dependency in build_dependencies:
    info(f"checking for {dependency} library")
    if not is_library_available(dependency):
        failed_dependencies.append(dependency)


binary_extension = ""
if current_platform == "windows":
    binary_extension = ".exe"

if len(failed_dependencies) > 0:
    error("missing build dependencies:")
    for dependency in failed_dependencies:
        print(f"\t * {dependency}")
    info("install these libraries using your package manager")
    exit(1)

glfw3 = get_library("glfw3")
rubberband = get_library("rubberband")
freetype2 = get_library("freetype2")
OpenImageIO = get_library("OpenImageIO")
ffmpeg = get_library(ffmpeg_libraries_list)
OpenColorIO = get_library("OpenColorIO")

global_compiler_flags.append(get_cflags(ffmpeg_libraries_list))
global_compiler_flags.append(get_cflags("OpenColorIO"))

global_compiler_flags.append(get_cflags("freetype2"))
if get_platform() == "linux":
    global_compiler_flags.append(get_cflags("gtk+-3.0"))

raster_ImGui                  = "-lraster_ImGui"
raster_avcpp                  = "-lraster_avcpp"
raster_font                   = "-lraster_font"
raster_common                 = "-lraster_common"
raster_audio                  = "-lraster_audio"
raster_image                  = "-lraster_image"
raster_gpu                    = "-lraster_gpu"
raster_compositor             = "-lraster_compositor"
raster_app                    = "-lraster_app"

nfd = "-lraster_nfd"

binary = 0
shared = 1
easing = 2
asset = 3
attribute = 4
node = 5
plugin = 6

ui_deps = [raster_common, raster_ImGui, raster_gpu, raster_compositor, raster_font, nfd]

build_environment = "dist/.build/"

build_modules = [
    ["ImGui", shared, [freetype2]],
    ["avcpp", shared, [ffmpeg]],
    ["font", shared],
    ["image", shared, [OpenImageIO, OpenColorIO]],
    ["gpu", shared, [glfw3, raster_ImGui, raster_image]],
    ["common", shared, [raster_ImGui, raster_gpu, raster_image, raster_font, raster_avcpp, ffmpeg, OpenColorIO, rubberband, nfd]],
    ["audio", shared, [raster_common, rubberband]],
    ["compositor", shared, [raster_gpu, raster_common]],
    ["core", binary, [raster_common, raster_ImGui, raster_gpu, raster_font, raster_compositor, nfd, raster_avcpp, ffmpeg, raster_audio, "-lbfd", "-lunwind"] + ["-ldbghelp"] if current_platform == "windows" else []],
    
    ["bezier_easing", easing, [raster_common, raster_ImGui]],
    ["constant_easing", easing, [raster_common, raster_ImGui]],
    ["bounce_easing", easing, [raster_common, raster_ImGui]],
    ["linear_easing", easing, [raster_common, raster_ImGui]],
    ["elastic_easing", easing, [raster_common, raster_ImGui]],
    ["random_easing", easing, [raster_common, raster_ImGui]],

    ["image_asset", asset, [raster_common, raster_gpu, raster_ImGui, raster_image]],
    ["placeholder_asset", asset, [raster_common, raster_gpu, raster_ImGui]],
    ["media_asset", asset, [raster_common, raster_gpu, raster_ImGui, raster_avcpp, ffmpeg]],
    ["folder_asset", asset, [raster_common, raster_ImGui]],

    ["float_attribute", attribute, [raster_common, raster_ImGui]],
    ["vec4_attribute", attribute, [raster_common, raster_ImGui]],
    ["transform2d_attribute", attribute, [raster_common, raster_ImGui]],
    ["asset_attribute", attribute, [raster_common, raster_ImGui]],
    ["color4_attribute", attribute, [raster_common, raster_ImGui]],
    ["vec3_attribute", attribute, [raster_common, raster_ImGui]],
    ["vec2_attribute", attribute, [raster_common, raster_ImGui]],
    ["gradient1d_attribute", attribute, [raster_common, raster_ImGui]],
    ["line2d_attribute", attribute, [raster_common, raster_ImGui]],
    ["folder_attribute", attribute, [raster_common, raster_ImGui]],
    ["bezier_attribute", attribute, [raster_common, raster_ImGui]],
    ["convolution_kernel_attribute", attribute, [raster_common, raster_ImGui, raster_gpu]],
    ["colorspace_attribute", attribute, [raster_common, raster_ImGui]],
    ["camera_attribute", attribute, [raster_common, raster_ImGui]],

    ["preferences", plugin, [raster_common]],
    ["xml_effects", plugin, [raster_common, raster_font, raster_gpu, raster_compositor, raster_ImGui]],
    ["matchbox_effects", plugin, [raster_common, raster_gpu, raster_compositor, raster_image, raster_ImGui]],
    ["rendering", plugin, [raster_common, raster_gpu, raster_image, raster_ImGui, raster_font]],
    ["ui", plugin, ui_deps],

    ["audio/decode_audio_asset", node, [raster_common, raster_avcpp, raster_audio, ffmpeg]],
    ["audio/export_to_audio_bus", node, [raster_common, raster_audio]],
    ["audio/bass_treble_effect", node, [raster_common, raster_audio]],
    ["audio/echo_effect", node, [raster_common, raster_audio]],
    ["audio/reverb_effect", node, [raster_common, raster_audio]],
    ["audio/mix_audio_samples", node, [raster_common, raster_audio]],
    ["audio/merge_audio_samples", node, [raster_common, raster_audio]],
    ["audio/audio_waveform_sine", node, [raster_common, raster_audio]],
    ["audio/audio_waveform_square", node, [raster_common, raster_audio]],
    ["audio/amplify_audio", node, [raster_common, raster_audio]],
    ["audio/pitch_shift_audio", node, [raster_common, raster_audio]],

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
    ["utilities/dummy", node, [raster_common]],

    ["rendering/export_renderable", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/layer2d", node, [raster_common, raster_gpu, raster_compositor, raster_ImGui]],
    ["rendering/solid2d", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/echo", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/tracking_motion_blur", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/merge", node, [raster_common, raster_gpu, raster_compositor, raster_ImGui, raster_font]],
    ["rendering/decode_video_asset", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/line2d", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/split_channels", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/combine_channels", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/bezier2d", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/convolve", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/ocio_grading_primary_transform", node, [raster_common, raster_gpu, raster_compositor, OpenColorIO]],
    ["rendering/ocio_colorspace_transform", node, [raster_common, raster_gpu, raster_compositor, OpenColorIO]],
    ["rendering/rasterize", node, [raster_common, raster_gpu, raster_compositor]],
    ["rendering/basic_perspective", node, [raster_common, raster_gpu, raster_compositor]],

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

pak_targets = [
    ["core", build_environment, "dist/paks/"],
    ["core_data", "src/misc/core_data", "dist/paks/"],
]

required_folders = [
    "nodes", "easings", "assets", "attributes", "plugins"
]

def ensure_folder_exists(path):
    if path_exists(path):
        rmdir(path)
    if not path_exists(path):
        mkdir(path)

def clean_folder(path):
    if path_exists(path):
        rmdir(path)
    ensure_folder_exists(path)

@entry_point(default=True)
def build():
    info("building raster")

    ensure_folder_exists("dist/")
    ensure_folder_exists("dist/paks/")
    ensure_folder_exists(build_environment)

    write_build_number()
    check_required_folders()
    build_pch()
    build_nfd()
    for module in build_modules:
        build_module(module)

    move_core_libraries()
    if path_exists("src/misc/matchbox_shaders.pak"):
        info("moving matchbox_shaders.pak to dist/paks/")
        if path_exists("dist/paks/matchbox_shaders.pak"):
            rm("dist/paks/matchbox_shaders.pak")
        shutil.copy("src/misc/matchbox_shaders.pak", "dist/paks/matchbox_shaders.pak")
    build_starter()
    create_paks()

    rmdir(build_environment)

def build_pch():
    info("compiling include/raster.h (include/raster.g.gch)")
    compile_file("include/raster.h", custom_output_path="include/raster.h.gch", pch_cxx=True, progress=1, files_count=1)

def create_paks():
    for pak in pak_targets:
        build_pak(pak)

def build_pak(pak):
    name = pak[0]
    input = pak[1]
    output = pak[2]
    create_pak_from_directory(name, input, output)

def build_starter():
    info("building starter")
    starter_files = glob_files("src/starter/", "cpp") + glob_files("src/starter/", "c")
    starter_deps = [

    ]
    starter_objects = compile_files(starter_files)
    link_executable(starter_objects, executable_name="dist/starter", local_linker_flags=starter_deps, cxx=True)

def build_nfd():
    info("building NFD")
    nfd_files = ["src/nfd/nfd_win.cpp" if get_platform() == "windows" else "src/nfd/nfd_gtk.cpp"]
    nfd_deps = []
    if get_platform() == "linux":
        nfd_deps.append(get_library("gtk+-3.0"))
    if get_platform() == "windows":
        nfd_deps.append("-lOle32")
        nfd_deps.append("-luuid")

    nfd_objects = compile_files(nfd_files)
    link_shared_library(nfd_objects, executable_name="raster_nfd", local_linker_flags=nfd_deps, cxx=True)

def move_core_libraries():
    core_files = glob_files(".", shared_library_extension, False)
    for file in core_files:
        mv(file, build_environment + os.path.basename(file))

def build_module(module):
    module_name = module[0]
    module_type = module[1]
    info(f"building module {module_name}")

    folder_name = get_folder_name_by_module_type(module_type)
    executable_type = "executable" if module_type == binary else "shared"

    glob_path = f"src/{module_name}/"
    if folder_name is not None:
        glob_path = f"src/{folder_name}/{module_name}"
    module_files = glob_files(glob_path, "cpp") + glob_files(glob_path, "c")
    
    linker_flags = []
    if len(module) > 2:
        linker_flags += module[2]
    objects = compile_files(module_files)
    output_path = "raster_" + module_name
    if folder_name is not None:
        output_path = f"raster_{hash_string(module_name)[:10]}"
    link_objects(objects, output_path, linker_flags, executable_type=executable_type, cxx=True)

    if folder_name is not None:
        mv(process_shared_library_name(output_path), f"{build_environment}{folder_name}/{path_basename(process_shared_library_name(output_path))}")
    if module_type == binary:
        mv(f"{output_path}{binary_extension}", f"{build_environment}/{output_path}{'' if len(executale_extension) == 0 else '.' + executale_extension}")

def get_folder_name_by_module_type(module_type):
    if module_type == node:
        return "nodes"
    elif module_type == easing:
        return "easings"
    elif module_type == asset:
        return "assets"
    elif module_type == attribute:
        return "attributes"
    elif module_type == plugin:
        return "plugins"
    return None

def check_required_folders():
    for folder in required_folders:
        if not path_exists(build_environment + folder + "/"):
            mkdir(build_environment + folder + "/")
            info(f"creating {folder}/ folder")

def remove_required_folders():
    for folder in required_folders:
        if path_exists(build_environment + folder + "/"):
            rmdir(build_environment + folder + "/")
            info(f"removing {folder}/ folder")

def write_build_number():
    bn_content = open("include/build_number.h", "r").read()
    with open("include/build_number.h", "w+") as bn_file:
        bn_lines = bn_content.split("\n")
        bn_second_parts = bn_lines[1].split(" ")
        incremented_number = int(bn_second_parts[2]) + 1
        bn_file.write("#pragma once\n#define BUILD_NUMBER " + str(incremented_number))
        info(f"current build number: {incremented_number}")

def create_pak_from_directory(name, input, output):
    info("creating pak from " + input)
    shutil.make_archive(name, 'zip', input)
    if not path_exists(output + "/" + name +  '.pak'):
        create_file(output + "/" + name +  '.pak')
    mv(name + '.zip', output + "/" + name +  '.pak')

@entry_point
def clean():
    remove_required_folders()
    clean_cache()

@entry_point
def full_rebuild():
    clean()
    build()


router()
