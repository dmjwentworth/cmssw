import os
import argparse
import subprocess

SAMPLE_KEY = {
    "SingleElectron": "34002.88",
    "TTbar": "34034.88",
    "DYToLL": "34044.88",
    "DYToTauTau": "34045.88",
    "ZMM": "34050.88",
    "H125_diphoton": "34052.88",
    "VBFHZZ4Nu": "34131.88",
}


def parse_args():
    parser = argparse.ArgumentParser(description="Generate sample configurations for physics analysis.")
    parser.add_argument("--sample", type=str, required=True, help="Sample name to generate configuration for.")
    parser.add_argument("--outputDir", type=str, required=True, help="Output directory name for the generated configuration.")
    parser.add_argument("--nEvents", type=int, help="Number of events to generate.")
    parser.add_argument("--useCondor", action="store_true", help="Flag to indicate whether to use Condor for job submission.")
    return parser.parse_args()


def create_shell_script(args):
    sample = SAMPLE_KEY.get(args.sample)
    shell_script_path = os.path.join(args.outputDir, f"run_{args.sample}.sh")
    os.makedirs(args.outputDir, exist_ok=True)
    abs_output_dir = os.path.abspath(args.outputDir)
    with open(shell_script_path, "w") as f:
        f.write("#!/bin/bash\n")
        f.write(f"runTheMatrix.py -l {sample} --what upgrade --nEvents {args.nEvents} --nThreads 4\n")
        f.write(f"mv {sample}_*/ {abs_output_dir}/\n")
    subprocess.run(["chmod", "+x", shell_script_path])
    return shell_script_path


def submit_to_condor(shell_script_path):
    shell_script = os.path.basename(shell_script_path)
    condor_script = shell_script.replace(".sh", ".sub")
    condor_script_path = shell_script_path.replace(".sh", ".sub")
    with open(condor_script_path, "w") as f:
        f.write("getenv = true\n")
        f.write(f"executable = {shell_script}\n")
        f.write("output = $(ClusterId).$(ProcId).out\n")
        f.write("error = $(ClusterId).$(ProcId).err\n")
        f.write("log = $(ClusterId).$(ProcId).log\n")
        f.write("+MaxRuntime = 144000\n")
        f.write("queue\n")
    subprocess.run(["condor_submit", condor_script], cwd=args.outputDir)


def main(args):
    shell_script_path = create_shell_script(args)
    if args.useCondor:
        submit_to_condor(shell_script_path)
    else:
        shell_script = os.path.basename(shell_script_path)
        subprocess.run([shell_script], cwd=args.outputDir)


if __name__ == "__main__":
    args = parse_args()
    main(args)

