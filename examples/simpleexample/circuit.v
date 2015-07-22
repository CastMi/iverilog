module mysubmod (w, d, q);

input w;
input d;
output q;

assign q = w || d;

endmodule

module mymod (a, b, c, d, y);

input a;
input b;
input c;
input d;
output y;

wire a;
wire b;
wire c;
wire d;
wire y;

wire out_0;
wire out_1;
wire g;
wire h;

mysubmod instance_1(a, b, g);
mysubmod instance_2(a, c, h);

and u0 (out_0, a, b);

or u1 (out_1, c, d);

or u2 (y, out_0, out_1);

endmodule
