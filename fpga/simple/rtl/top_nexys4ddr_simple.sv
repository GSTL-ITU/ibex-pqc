// Nexys 4 DDR (= Nexys A7-100T) top for the FPGA version of the Ibex Simple
// System. 100 MHz board clock -> 50 MHz system clock (clkgen_xil7series, the
// same clock as the demo system), CPU RESET button, LEDs:
//   LED[7:0]  last character the program wrote to the output register
//   LED[15]   program requested a halt (sim_halt() / return from main)
//
// The core parameters are set on this top level (FuseSoC passes them to the
// top only, e.g. from util/ibex_config.py) and forwarded to the system.
// RV32M, RV32B, RV32ZC, RegFile and BaseIsa come in as defines, as in the
// simulation system.
module top_nexys4ddr_simple #(
  parameter bit          SecureIbex       = 1'b0,
  parameter int unsigned LockstepOffset   = 1,
  parameter bit          ICacheScramble   = 1'b0,
  parameter bit          PMPEnable        = 1'b0,
  parameter int unsigned PMPGranularity   = 0,
  parameter int unsigned PMPNumRegions    = 4,
  parameter int unsigned MHPMCounterNum   = 0,
  parameter int unsigned MHPMCounterWidth = 40,
  parameter bit          RV32E            = 1'b0,
  parameter bit          BranchTargetALU  = 1'b0,
  parameter bit          WritebackStage   = 1'b0,
  parameter bit          ICache           = 1'b0,
  parameter bit          DbgTriggerEn     = 1'b0,
  parameter bit          ICacheECC        = 1'b0,
  parameter bit          BranchPredictor  = 1'b0,
  parameter              SRAMInitFile     = "",
  parameter int unsigned MemSizeKiB       = 128
) (
  input         IO_CLK,
  input         IO_RST_N,
  output [15:0] LED
);

  logic clk_sys, rst_sys_n;
  logic [7:0] char;
  logic halted;

  ibex_simple_system_fpga #(
    .SecureIbex      (SecureIbex),
    .LockstepOffset  (LockstepOffset),
    .ICacheScramble  (ICacheScramble),
    .PMPEnable       (PMPEnable),
    .PMPGranularity  (PMPGranularity),
    .PMPNumRegions   (PMPNumRegions),
    .MHPMCounterNum  (MHPMCounterNum),
    .MHPMCounterWidth(MHPMCounterWidth),
    .RV32E           (RV32E),
    .BranchTargetALU (BranchTargetALU),
    .WritebackStage  (WritebackStage),
    .ICache          (ICache),
    .DbgTriggerEn    (DbgTriggerEn),
    .ICacheECC       (ICacheECC),
    .BranchPredictor (BranchPredictor),
    .SRAMInitFile    (SRAMInitFile),
    .MemSizeKiB      (MemSizeKiB)
  ) u_ibex_simple_system (
    .clk_sys_i (clk_sys),
    .rst_sys_ni(rst_sys_n),
    .char_o    (char),
    .halted_o  (halted)
  );

  assign LED = {halted, 7'b0, char};

  clkgen_xil7series clkgen (
    .IO_CLK,
    .IO_RST_N,
    .clk_sys,
    .rst_sys_n
  );

endmodule
