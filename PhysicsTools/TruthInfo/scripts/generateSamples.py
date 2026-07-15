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
    "TenTau": "34087.88",
    "SingleTop": "34999.88",
    "TTbarPowheg": "34998.88",
    "Diboson": "34997.88",
    "VH": "34996.88",
}


def parse_args():
    parser = argparse.ArgumentParser(description="Generate sample configurations for physics analysis.")
    parser.add_argument("--sample", type=str, required=True, help="Sample name to generate configuration for.")
    parser.add_argument("--outputDir", type=str, required=True, help="Output directory name for the generated configuration.")
    parser.add_argument("--nEvents", type=int, help="Number of events to generate.")
    parser.add_argument("--nThreads", type=int, default=1, help="Number of threads to use in runTheMatrix.py.")
    args = parser.parse_args()
    
    if args.sample not in SAMPLE_KEY:
        raise ValueError(f"Sample '{args.sample}' is not recognized. Available samples: {list(SAMPLE_KEY.keys())}")
    return args


def create_shell_script(args):
    sample = SAMPLE_KEY.get(args.sample)
    submission_dir = os.path.join("submissions", os.path.basename(args.outputDir))
    shell_script_path = os.path.join(submission_dir, f"{args.sample}.sh")
    abs_output_dir = os.path.abspath(args.outputDir)
    os.makedirs(submission_dir, exist_ok=True)
    os.makedirs(abs_output_dir, exist_ok=True)
   
    with open(shell_script_path, "w") as f:
        f.write("#!/bin/bash\n")
        f.write(f"runTheMatrix.py -l {sample} --what upgrade --nEvents {args.nEvents} --nThreads {args.nThreads}\n")
        f.write(f"mv {sample}_*/ {abs_output_dir}/\n")
   
    subprocess.run(["chmod", "+x", shell_script_path])
    return shell_script_path


def submit_to_condor(shell_script_path):
    submission_dir = os.path.dirname(shell_script_path)
    shell_script = os.path.basename(shell_script_path)
    condor_script = shell_script.replace(".sh", ".sub")
    condor_script_path = shell_script_path.replace(".sh", ".sub")
    with open(condor_script_path, "w") as f:
        f.write("getenv = true\n")
        f.write(f"executable = {shell_script}\n")
        f.write("output = $(ClusterId).$(ProcId).out\n")
        f.write("error = $(ClusterId).$(ProcId).err\n")
        f.write("log = $(ClusterId).$(ProcId).log\n")
        f.write(f"request_cpus = {args.nThreads}\n")
        f.write(f"request_disk = {int(args.nEvents * 5)} M\n")  # 5MB per event
        f.write("+MaxRuntime = 144000\n")
        f.write("queue\n")
    subprocess.run(["condor_submit", condor_script], cwd=submission_dir)


def main(args):
    shell_script_path = create_shell_script(args)
    submit_to_condor(shell_script_path)


if __name__ == "__main__":
    args = parse_args()
    main(args)

