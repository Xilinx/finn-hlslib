module dup_tb;

	localparam int DATA_W = 32;

	// Global Control
	logic  clk = 0;
	always #5ns clk = !clk;
	logic rst = 1;
	initial begin
		repeat(12) @(posedge clk);
		rst <= 0;
	end

	logic [DATA_W-1:0]    src_TDATA;
	logic                 src_TVALID;
	uwire                 src_TREADY;

	uwire [DATA_W-1:0]    dst_0_TDATA;
	uwire                 dst_0_TVALID;
	uwire                 dst_0_TREADY = 1;

	uwire [DATA_W-1:0]    dst_1_TDATA;
	uwire                 dst_1_TVALID;
	uwire                 dst_1_TREADY = 1;

	uwire [DATA_W-1:0]    dst_2_TDATA;
	uwire                 dst_2_TVALID;
	uwire                 dst_2_TREADY = 1;

	uwire [DATA_W-1:0]    dst_3_TDATA;
	uwire                 dst_3_TVALID;
	uwire                 dst_3_TREADY = 1;

	uwire [DATA_W-1:0]    dst_4_TDATA;
	uwire                 dst_4_TVALID;
	uwire                 dst_4_TREADY = 1;

	dup_top dut (
		.ap_clk     (clk),
		.ap_rst_n   (!rst),
		.src_TVALID (src_TVALID),
		.src_TREADY (src_TREADY),
		.src_TDATA  (src_TDATA),
		.dst_0_TDATA(dst_0_TDATA),
		.dst_1_TDATA(dst_1_TDATA),
		.dst_2_TDATA(dst_2_TDATA),
		.dst_3_TDATA(dst_3_TDATA),
		.dst_4_TDATA(dst_4_TDATA),
		.dst_0_TVALID(dst_0_TVALID),
		.dst_0_TREADY(dst_0_TREADY),
		.dst_1_TVALID(dst_1_TVALID),
		.dst_1_TREADY(dst_1_TREADY),
		.dst_2_TVALID(dst_2_TVALID),
		.dst_2_TREADY(dst_2_TREADY),
		.dst_3_TVALID(dst_3_TVALID),
		.dst_3_TREADY(dst_3_TREADY),
		.dst_4_TVALID(dst_4_TVALID),
		.dst_4_TREADY(dst_4_TREADY)
	);

	task send_word(input logic [DATA_W-1:0] data);
		src_TDATA  <= data;
		src_TVALID <= 1;
		@(posedge clk iff src_TREADY);
		src_TVALID <=  0;
		src_TDATA  <= 'x;
	endtask

	always @(posedge clk iff !rst) begin
		if (dst_0_TVALID && dst_0_TREADY) begin
			$display("%0t dst0=0x%08x", $time, dst_0_TDATA);
		end
		if (dst_1_TVALID && dst_1_TREADY) begin
			$display("%0t dst1=0x%08x", $time, dst_1_TDATA);
		end
		if (dst_2_TVALID && dst_2_TREADY) begin
			$display("%0t dst2=0x%08x", $time, dst_2_TDATA);
		end
		if (dst_3_TVALID && dst_3_TREADY) begin
			$display("%0t dst3=0x%08x", $time, dst_3_TDATA);
		end
		if (dst_4_TVALID && dst_4_TREADY) begin
			$display("%0t dst4=0x%08x", $time, dst_4_TDATA);
		end
	end

	initial begin
		@(posedge clk iff !rst);

		for(int unsigned  i = 0; i < 300; i++)  send_word(i);
		send_word(32'hDEAD_BEEF);

		repeat(10) @(posedge clk);
		$finish;
	end

endmodule : dup_tb
