# Area (and optionally timing) reports for a FuseSoC-generated Vivado project.
#
#   vivado -mode batch -source area.tcl -tclargs <work_dir> <project> <out_dir> <impl> <jobs>
#
# <work_dir>  directory with <project>.tcl (written by `fusesoc run --setup`)
# <out_dir>   where the reports go
# <impl>      0: synthesis + opt_design; 1: also place and route + timing
#
# Reports (in <out_dir>):
#   util_<stage>.rpt       utilization of the whole design
#   util_hier_<stage>.rpt  utilization per hierarchy level (core vs SoC)
#   timing_route.rpt       timing summary (impl=1 only)
# <stage> is "opt" (after synthesis and opt_design) or "route" (impl=1).

lassign $argv work_dir project out_dir impl jobs

cd $work_dir
file mkdir $out_dir

# Creates the project (sources, generics, defines, constraints)
source ${project}.tcl

# The simple system's FuseSoC dependencies list ibex/shared/rtl/ram_2p.sv twice
# (sim_shared and fpga_xilinx_shared). Both copies are the same file.
set_msg_config -id {Synth 8-9873} -string {ram_2p} -new_severity INFO

reset_runs synth_1
launch_runs synth_1 -jobs $jobs
wait_on_run synth_1
if {[get_property PROGRESS [get_runs synth_1]] != "100%"} {
  puts "ERROR: synthesis failed, see [get_property DIRECTORY [get_runs synth_1]]/runme.log"
  exit 1
}
open_run synth_1

proc write_reports {out_dir stage} {
  report_utilization -file $out_dir/util_${stage}.rpt
  report_utilization -hierarchical -hierarchical_depth 8 \
    -file $out_dir/util_hier_${stage}.rpt
}

# opt_design removes logic without loads and propagates constants, like the
# implementation flow does, so the numbers are what really ends up on the FPGA
opt_design
write_reports $out_dir opt

if {$impl} {
  place_design
  route_design
  write_reports $out_dir route
  report_timing_summary -file $out_dir/timing_route.rpt
}

puts "INFO: reports written to $out_dir"
