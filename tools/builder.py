#!/usr/bin/env python3

import argparse
import subprocess
import re
from pathlib import Path

prog = 'builder'

version = '1.0.0'

def get_platformio_envs():
    """Retrieve list of environments from platformio.ini or through platformio CLI."""
    ini_path = Path('platformio.ini')
    envs = []
    if ini_path.exists():
        with open(ini_path, 'r') as file:
            content = file.read()
            envs = re.findall(r'\[env:(.+?)\]', content)
    else:
        # Fallback to using platformio CLI if platformio.ini is not found
        result = subprocess.run(['platformio', 'project', 'data'], capture_output=True, text=True)
        if result.returncode == 0:
            # Parse output to extract envs
            # This is a placeholder, actual implementation depends on the output format of 'platformio project data'
            pass
    return envs

def list_targets():
    """List all available target environments."""
    envs = get_platformio_envs()
    print("Available target environments:")
    for env in envs:
        print(f"  - {env}")

def run_command(command, target, deploy = False, suite = None, clean = False):
    """Run the appropriate platformio command based on input parameters."""
    cmd = ['platformio', command]
    if command in ['run', 'test']:
        cmd += ['-e', target]
        if deploy and command == 'run':
            cmd += ['-t', 'upload']
        if suite and command == 'test':
            cmd += ['-f', suite]


    print(f"\033[1;93m RadioMesh builder: executing {' '.join(cmd)}\033[0m")
    # remove .pio folder before building
    if clean:
        subprocess.run(['rm', '-rf', '.pio'])

    try:
        subprocess.check_call(cmd)
    except subprocess.CalledProcessError:
        print(f"Error: {' '.join(cmd)} failed.")
        exit(1)

def main():
    parser = argparse.ArgumentParser(description='builder - A PlatformIO CLI Wrapper to build RadioMesh library and examples.')
    subparsers = parser.add_subparsers(dest='command', help=f'Available commands')

    # Version command
    version_parser = subparsers.add_parser('version', help='Print cli version')

    # Build/test command
    for cmd in ['build', 'test', 'clean']:

        cmd_parser = subparsers.add_parser(cmd, help=f'{cmd} the project for a specific target')
        cmd_parser.add_argument('-t', '--target', required=True, help='Target environment to build/test/clean')
        if cmd == 'build':
            cmd_parser.add_argument('--deploy', action='store_true', help='Deploy after build')
            cmd_parser.add_argument('--clean', action='store_true', help='Clean before build')
        elif cmd == 'test':
            cmd_parser.add_argument('-s', '--suite', help='Test suite to run')

    # List targets command
    list_parser = subparsers.add_parser('list-targets', help='list all available target environments')

    args = parser.parse_args()

    if args.command in ['build', 'test']:
        # Validate target
        envs = get_platformio_envs()
        if args.target not in envs:
            print(f"Error: Target '{args.target}' is not a valid environment. Available environments are: {', '.join(envs)}")
            exit(1)

        # Map command from warp to platformio

        # pio_command = 'run' if args.command == 'build' or 'clean' else 'test'
        if args.command == 'build' or args.command == 'clean':
            pio_command = 'run'
        elif args.command == 'test':
            pio_command = 'test'
        else:
            pio_command = args.command

        run_command(pio_command, args.target,
                    getattr(args, 'deploy', False),
                    getattr(args, 'suite', None),
                    getattr(args, 'clean', False))

    elif args.command == 'list-targets':
        list_targets()
    elif args.command == 'version':
        print(f'builder cli: v{version}')
    else:
        parser.print_help()

if __name__ == "__main__":
    main()
