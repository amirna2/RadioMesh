import os
import subprocess

Import("env")

def build_static_lib(source, target, env):
    print("Building static library")

    # get the project name
    project_name = os.path.basename(env.subst("$PROJECT_DIR"))

    # build directory
    build_dir = env.subst("$BUILD_DIR")
    # build object files directory
    build_dir_src = env.subst("$BUILD_DIR/src")

    print(f"Project name: {project_name}")
    print(f"Build directory: {build_dir_src}")

    # get all the .o files from the build directory
    objects = []
    for root, dirs, files in os.walk(build_dir_src):
        for file in files:
            if file.endswith(".o"):
                objects.append(os.path.join(root, file))

    print(f"Objects: {objects}")

    # archive the .o files into a static library
    cmd = ["ar", "rcs", f"{build_dir}/lib{project_name}.a"] + objects
    print(" ".join(cmd))
    try:
        subprocess.check_call(cmd)
    except subprocess.CalledProcessError:
        print(f"Error: {' '.join(cmd)} failed.")
        exit(1)



env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", build_static_lib)
