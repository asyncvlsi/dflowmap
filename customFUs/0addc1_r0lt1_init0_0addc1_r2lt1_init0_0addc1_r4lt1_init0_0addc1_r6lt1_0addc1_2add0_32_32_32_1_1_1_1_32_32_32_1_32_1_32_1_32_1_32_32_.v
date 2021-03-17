module op (clk, x0, x1, x2, out0, out1, out2, out3, out4, out5);
output reg [0:0] out0;
output reg [0:0] out1;
output reg [0:0] out2;
output reg [0:0] out3;
output reg [31:0] out4;
output reg [31:0] out5;

input  [31:0] x0;
input  [31:0] x1;
input  [31:0] x2;

input clk;

always @(posedge clk)
begin
  out0 <= (x0+1) < x1;
  out1 <= (x0+1) < x1;
  out2 <= (x0+1) < x1;
  out3 <= (x0+1) < x1;
  out4 <= x0+1;
  out5 <= x2 + x0;
end

endmodule