//
// Created by nfiege on 9/26/22.
//

#include "scm.h"
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <chrono>

scm::scm(int C, int timeout, bool quiet) : C(C), timeout(timeout), output_shift(0), quiet(quiet) {
	// make it even and count shift
	while ((this->C & 1) == 0) {
		this->C = this->C >> 1;
		this->output_shift++;
	}
	// set word sizes
	this->word_size = this->ceil_log2(this->C)+1;
	this->max_shift = this->word_size;
	this->shift_word_size = this->ceil_log2(this->max_shift+1);
}

void scm::solve() {
	if (!this->quiet) std::cout << "trying to solve SCM problem for constant " << this->C << " with word size " << this->word_size << " and max shift " << this->max_shift << std::endl;
	if (this->C == 1) {
		this->found_solution = true;
		this->ran_into_timeout = false;
		this->output_values[0] = 1;
		return;
	}
	while (!this->found_solution) {
		auto start_time = std::chrono::steady_clock::now();
		++this->num_adders;
		if (!this->quiet) std::cout << "  constructing problem for " << this->num_adders << " adders" << std::endl;
		this->construct_problem();
		if (!this->quiet) std::cout << "  start solving with " << this->variable_counter << " variables and " << this->constraint_counter << " constraints" << std::endl;
		auto [a, b] = this->check();
		auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count() / 1000.0;
		this->found_solution = a;
		this->ran_into_timeout = b;
		if (this->found_solution) {
			std::cout << "  found solution for #adders = " << this->num_adders << " after " << elapsed_time << " seconds 8-)" << std::endl;
			this->get_solution_from_backend();
		}
		else if (this->ran_into_timeout) {
			std::cout << "  ran into timeout for #adders = " << this->num_adders << " after " << elapsed_time << " seconds :-(" << std::endl;
		}
		else {
			std::cout << "  problem for #adders = " << this->num_adders << " is proven to be infeasible after " << elapsed_time << " seconds... keep trying :-)" << std::endl;
		}
	}
}

void scm::reset_backend() {
	throw std::runtime_error("reset_backend is impossible in base class");
}

void scm::construct_problem() {
	if (!this->quiet) std::cout << "    resetting backend now" << std::endl;
	this->reset_backend();
	if (!this->quiet) std::cout << "    creating variables now" << std::endl;
	this->create_variables();
	if (!this->quiet) std::cout << "    creating constraints now" << std::endl;
	this->create_constraints();
}

void scm::create_variables() {
	this->variable_counter = 0;
	if (!this->quiet) std::cout << "      creating constant zero variables" << std::endl;
	this->create_constant_zero_variable();
	if (!this->quiet) std::cout << "      creating input node variables" << std::endl;
	this->create_input_node_variables();
	for (int i=1; i<=this->num_adders; i++) {
		if (!this->quiet) std::cout << "      creating variables for node " << i << std::endl;
		if (!this->quiet) std::cout << "        create_input_select_mux_variables" << std::endl;
		this->create_input_select_mux_variables(i);
		if (!this->quiet) std::cout << "        create_input_select_selection_variables" << std::endl;
		this->create_input_select_selection_variables(i);
		if (!this->quiet) std::cout << "        create_input_shift_select_variable" << std::endl;
		this->create_input_shift_select_variable(i);
		if (!this->quiet) std::cout << "        create_shift_select_output_variables" << std::endl;
		this->create_shift_select_output_variables(i);
		if (!this->quiet) std::cout << "        create_input_shift_value_variables" << std::endl;
		this->create_input_shift_value_variables(i);
		if (!this->quiet) std::cout << "        create_shift_internal_variables" << std::endl;
		this->create_shift_internal_variables(i);
		if (!this->quiet) std::cout << "        create_input_negate_select_variable" << std::endl;
		this->create_input_negate_select_variable(i);
		if (!this->quiet) std::cout << "        create_negate_select_output_variables" << std::endl;
		this->create_negate_select_output_variables(i);
		if (!this->quiet) std::cout << "        create_input_negate_value_variable" << std::endl;
		this->create_input_negate_value_variable(i);
		if (!this->quiet) std::cout << "        create_xor_output_variables" << std::endl;
		this->create_xor_output_variables(i);
		if (!this->quiet) std::cout << "        create_adder_internal_variables" << std::endl;
		this->create_adder_internal_variables(i);
		if (!this->quiet) std::cout << "        create_output_value_variables" << std::endl;
		this->create_output_value_variables(i);
	}
}

void scm::create_constraints() {
	this->constraint_counter = 0;
	if (!this->quiet) std::cout << "      create_input_output_constraints" << std::endl;
	this->create_input_output_constraints();
	for (int i=1; i<=this->num_adders; i++) {
		if (!this->quiet) std::cout << "      creating constraints for node " << i << std::endl;
		if (!this->quiet) std::cout << "        create_input_select_constraints" << std::endl;
		this->create_input_select_constraints(i);
		if (!this->quiet) std::cout << "        create_input_select_limitation_constraints" << std::endl;
		this->create_input_select_limitation_constraints(i);
		if (!this->quiet) std::cout << "        create_shift_limitation_constraints" << std::endl;
		this->create_shift_limitation_constraints(i);
		if (!this->quiet) std::cout << "        create_shift_select_constraints" << std::endl;
		this->create_shift_select_constraints(i);
		if (!this->quiet) std::cout << "        create_shift_constraints" << std::endl;
		this->create_shift_constraints(i);
		if (!this->quiet) std::cout << "        create_negate_select_constraints" << std::endl;
		this->create_negate_select_constraints(i);
		if (!this->quiet) std::cout << "        create_xor_constraints" << std::endl;
		this->create_xor_constraints(i);
		if (!this->quiet) std::cout << "        create_adder_constraints" << std::endl;
		this->create_adder_constraints(i);
	}
}

void scm::create_input_node_variables() {
	for (int i=0; i<this->word_size; i++) {
		this->output_value_variables[{0, i}] = ++this->variable_counter;
		this->create_new_variable(this->variable_counter);
	}
}

void scm::create_constant_zero_variable() {
	this->constant_zero_variable = ++this->variable_counter;
	this->create_new_variable(this->variable_counter);
	this->force_bit(this->constant_zero_variable, 0);
	this->constraint_counter++;
}

void scm::create_input_select_mux_variables(int idx) {
	if (idx == 1) return;
	auto select_word_size = this->ceil_log2(idx);
	auto num_muxs = (1 << select_word_size) - 1;
	for (auto &dir : input_directions) {
		for (int mux_idx = 0; mux_idx < num_muxs; mux_idx++) {
			for (int w = 0; w < this->word_size; w++) {
				this->input_select_mux_variables[{idx, dir, mux_idx, w}] = ++this->variable_counter;
				this->create_new_variable(this->variable_counter);
			}
		}
	}
}

void scm::create_input_select_selection_variables(int idx) {
	if (idx == 1) return;
	auto select_word_size = this->ceil_log2(idx);
	for (auto &dir : input_directions) {
		for (int w = 0; w < select_word_size; w++) {
			this->input_select_selection_variables[{idx, dir, w}] = ++this->variable_counter;
			this->create_new_variable(this->variable_counter);
		}
	}
}

/*void scm::create_input_value_variables(int idx) {
	if (idx == 1) return;
	for (auto &dir : input_directions) {
		for (int w = 0; w < this->word_size; w++) {
			this->input_value_variables[{idx, dir, w}] = ++this->variable_counter;
			this->create_new_variable(this->variable_counter);
		}
	}
}*/

void scm::create_input_shift_select_variable(int idx) {
	if (idx == 1) return;
	this->input_shift_select_variables[idx] = ++this->variable_counter;
	this->create_new_variable(this->variable_counter);
}

void scm::create_shift_select_output_variables(int idx) {
	if (idx == 1) return;
	for (auto &dir : input_directions) {
		for (int w = 0; w < this->word_size; w++) {
			this->shift_select_output_variables[{idx, dir, w}] = ++this->variable_counter;
			this->create_new_variable(this->variable_counter);
		}
	}
}

void scm::create_input_shift_value_variables(int idx) {
	for (int w = 0; w < this->shift_word_size; w++) {
		this->input_shift_value_variables[{idx, w}] = ++this->variable_counter;
		this->create_new_variable(this->variable_counter);
	}
}

void scm::create_shift_internal_variables(int idx) {
	for (int mux_stage = 0; mux_stage < this->shift_word_size; mux_stage++) {
		for (int w = 0; w < this->word_size; w++) {
			this->shift_internal_mux_output_variables[{idx, mux_stage, w}] = ++this->variable_counter;
			if (mux_stage == this->shift_word_size-1) {
				this->shift_output_variables[{idx, w}] = this->variable_counter;
			}
			this->create_new_variable(this->variable_counter);
		}
	}
}

void scm::create_input_negate_select_variable(int idx) {
	this->input_negate_select_variables[idx] = ++this->variable_counter;
	this->create_new_variable(this->variable_counter);
}

void scm::create_negate_select_output_variables(int idx) {
	for (auto &dir : input_directions) {
		for (int w = 0; w < this->word_size; w++) {
			this->negate_select_output_variables[{idx, dir, w}] = ++this->variable_counter;
			this->create_new_variable(this->variable_counter);
		}
	}
}

void scm::create_input_negate_value_variable(int idx) {
	this->input_negate_value_variables[idx] = ++this->variable_counter;
	this->create_new_variable(this->variable_counter);
}

void scm::create_xor_output_variables(int idx) {
	for (int w = 0; w < this->word_size; w++) {
		this->xor_output_variables[{idx, w}] = ++this->variable_counter;
		this->create_new_variable(this->variable_counter);
	}
}

void scm::create_adder_internal_variables(int idx) {
	for (int w = 0; w < this->word_size; w++) {
		this->adder_internal_variables[{idx, w}] = ++this->variable_counter;
		this->create_new_variable(this->variable_counter);
	}
}

void scm::create_output_value_variables(int idx) {
	for (int w = 0; w < this->word_size; w++) {
		this->output_value_variables[{idx, w}] = ++this->variable_counter;
		this->create_new_variable(this->variable_counter);
	}
}

int scm::ceil_log2(int n) {
	try {
		return this->ceil_log2_cache.at(n);
	}
	catch (std::out_of_range&) {
		int val;
		if (n > 0) val = std::ceil(std::log2(n));
		else val = -1;
		return this->ceil_log2_cache[n] = val;
	}
}

int scm::floor_log2(int n) {
	try {
		return this->floor_log2_cache.at(n);
	}
	catch (std::out_of_range&) {
		int val;
		if (n > 0) val = std::floor(std::log2(n));
		else val = -1;
		return this->floor_log2_cache[n] = val;
	}
}

void scm::create_new_variable(int idx) {
	(void) idx; // just do nothing -> should be overloaded by backend if a variable must be explicitly created
}

void scm::create_2x1_mux(int a, int b, int s, int o) {
	throw std::runtime_error("create_2x1_mux is impossible in base class");
}

void scm::create_1x1_equivalence(int x, int y) {
	throw std::runtime_error("create_1x1_equivalence is impossible in base class");
}

void scm::create_2x1_xor(int a, int b, int y) {
	throw std::runtime_error("create_2x1_xor is impossible in base class");
}

void scm::create_add_sum(int a, int b, int c_i, int s) {
	throw std::runtime_error("create_add_sum is impossible in base class");
}

void scm::create_add_carry(int a, int b, int c_i, int c_o) {
	throw std::runtime_error("create_add_carry is impossible in base class");
}

void scm::force_bit(int x, int val) {
	throw std::runtime_error("force_bit is impossible in base class");
}

void scm::forbid_number(const std::vector<int> &x, int num) {
	throw std::runtime_error("forbid_number is impossible in base class");
}

void scm::force_number(const std::vector<int> &x, int num) {
	throw std::runtime_error("force_number is impossible in base class");
}

std::pair<bool, bool> scm::check() {
	throw std::runtime_error("check is impossible in base class");
}

int scm::get_result_value(int var_idx) {
	throw std::runtime_error("get_result_value is impossible in base class");
}

void scm::create_input_output_constraints() {
	std::vector<int> input_bits(this->word_size);
	std::vector<int> output_bits(this->word_size);
	for (auto w=0; w<this->word_size; w++) {
		input_bits[w] = this->output_value_variables.at({0, w});
		output_bits[w] = this->output_value_variables.at({this->num_adders, w});
	}
	// force input to 1 and output to C
	this->force_number(input_bits, 1);
	this->constraint_counter += this->word_size;
	this->force_number(output_bits, this->C);
	this->constraint_counter += this->word_size;
}

void scm::create_input_select_constraints(int idx) {
	if (idx == 1) return; // stage 1 has no input MUX because it can only be connected to the input node with idx=0
	// create constraints for all muxs
	//std::cout << "creating input select constraints for node #" << idx << std::endl;
	auto select_word_size = this->ceil_log2(idx);
	auto num_possible_muxs = (1 << select_word_size) - 1;
	for (auto &dir : this->input_directions) {
		//std::cout << "  dir = " << (dir==scm::left?"left":"right") << std::endl;
		int connected_inputs_counter = 0;
		int mux_idx = 0;
		for (int mux_stage = 0; mux_stage < select_word_size; mux_stage++) {
			//std::cout << "    mux stage = " << mux_stage << std::endl;
			auto num_muxs_per_stage = (1 << mux_stage);
			auto mux_select_var_idx = this->input_select_selection_variables.at({idx, dir, select_word_size-mux_stage-1}); // mux_stage
			for (int mux_idx_in_stage = 0; mux_idx_in_stage < num_muxs_per_stage; mux_idx_in_stage++) {
				//std::cout << "      mux idx in stage = " << mux_idx_in_stage << std::endl;
				//std::cout << "      mux idx = " << mux_idx << std::endl;
				if (mux_stage == select_word_size-1) {
					// connect with another node output
					auto zero_input_node_idx = 2 * mux_idx_in_stage;
					auto one_input_node_idx = zero_input_node_idx + 1;
					if (zero_input_node_idx >= idx) zero_input_node_idx = idx-1;
					if (one_input_node_idx >= idx) one_input_node_idx = idx-1;
					//std::cout << "        zero input node idx = " << zero_input_node_idx << std::endl;
					//std::cout << "        one input node idx = " << one_input_node_idx << std::endl;
					for (int w = 0; w < this->word_size; w++) {
						auto mux_output_var_idx = this->input_select_mux_variables.at({idx, dir, mux_idx, w});
						auto zero_input_var_idx = this->output_value_variables.at({zero_input_node_idx, w});
						auto one_input_var_idx = this->output_value_variables.at({one_input_node_idx, w});
						if (zero_input_node_idx == one_input_node_idx) {
							// both inputs are equal -> mux output == mux input (select line does not matter...)
							this->create_1x1_equivalence(zero_input_node_idx, mux_output_var_idx);
						}
						else {
							this->create_2x1_mux(zero_input_var_idx, one_input_var_idx, mux_select_var_idx, mux_output_var_idx);
						}
						this->constraint_counter++;
					}
				}
				else {
					// connect with mux from higher stage
					auto num_muxs_in_next_stage = (1 << (mux_stage + 1));
					auto zero_mux_idx_in_next_stage = 2 * mux_idx_in_stage;
					auto zero_input_mux_idx = num_muxs_in_next_stage - 1 + zero_mux_idx_in_next_stage;
					//auto zero_input_mux_idx = mux_idx + num_muxs_per_stage;
					auto one_input_mux_idx = zero_input_mux_idx + 1;
					//std::cout << "        zero input mux idx = " << zero_input_mux_idx << std::endl;
					//std::cout << "        one input mux idx = " << one_input_mux_idx << std::endl;
					for (int w = 0; w < this->word_size; w++) {
						auto mux_output_var_idx = this->input_select_mux_variables.at({idx, dir, mux_idx, w});
						auto zero_input_var_idx = this->input_select_mux_variables.at({idx, dir, zero_input_mux_idx, w});
						auto one_input_var_idx = this->input_select_mux_variables.at({idx, dir, one_input_mux_idx, w});
						this->create_2x1_mux(zero_input_var_idx, one_input_var_idx, mux_select_var_idx, mux_output_var_idx);
						this->constraint_counter++;
					}
				}
				// increment current mux idx
				mux_idx++;
				//std::cout << std::endl;
			}
		}
	}
}

void scm::create_shift_constraints(int idx) {
	for (auto stage = 0; stage < this->shift_word_size; stage++) {
		auto shift_width = (1 << stage);
		auto select_input_var_idx = this->input_shift_value_variables.at({idx, stage});
		for (auto w = 0; w < this->word_size; w++) {
			auto w_prev = w - shift_width;
			auto connect_zero_const = w_prev < 0;
			int zero_input_var_idx;
			int one_input_var_idx;
			auto mux_output_var_idx = this->shift_internal_mux_output_variables.at({idx, stage, w});
			if (stage == 0) {
				// connect shifter inputs
				if (idx == 1) {
					// shifter input is the output of the input node with idx = 0
					zero_input_var_idx = this->output_value_variables.at({0, w});
					if (connect_zero_const) {
						one_input_var_idx = this->constant_zero_variable;
					}
					else {
						one_input_var_idx = this->output_value_variables.at({0, w_prev});
					}
				}
				else {
					// shifter input is the left shift-select-mux
					zero_input_var_idx = this->shift_select_output_variables.at({idx, scm::left, w});
					if (connect_zero_const) {
						one_input_var_idx = this->constant_zero_variable;
					}
					else {
						one_input_var_idx = this->shift_select_output_variables.at({idx, scm::left, w_prev});
					}
				}
			}
			else {
				// connect output of previous stage
				zero_input_var_idx = this->shift_internal_mux_output_variables.at({idx, stage-1, w});
				if (connect_zero_const) {
					one_input_var_idx = this->constant_zero_variable;
				}
				else {
					one_input_var_idx = this->shift_internal_mux_output_variables.at({idx, stage-1, w_prev});
				}
			}
			this->create_2x1_mux(zero_input_var_idx, one_input_var_idx, select_input_var_idx, mux_output_var_idx);
			this->constraint_counter++;
		}
	}
}

void scm::create_shift_select_constraints(int idx) {
	if (idx == 1) return; // this node doesn't need shift input muxs
	auto select_var = this->input_shift_select_variables.at(idx);
	for (auto &dir : this->input_directions) {
		for (auto w = 0; w < this->word_size; w++) {
			auto mux_output_var_idx = this->shift_select_output_variables.at({idx, dir, w});
			auto left_input_var_idx = this->input_select_mux_variables.at({idx, scm::left, 0, w});
			auto right_input_var_idx = this->input_select_mux_variables.at({idx, scm::right, 0, w});
			if (dir == scm::left) {
				// left mux has left input in b input and right input in a input
				this->create_2x1_mux(right_input_var_idx, left_input_var_idx, select_var, mux_output_var_idx);
				this->constraint_counter++;
			}
			else {
				// right mux has left input in '0' input and right input in '1' input
				this->create_2x1_mux(left_input_var_idx, right_input_var_idx, select_var, mux_output_var_idx);
				this->constraint_counter++;
			}
		}
	}
}

void scm::create_negate_select_constraints(int idx) {
	auto select_var_idx = this->input_negate_select_variables.at(idx);
	for (int w = 0; w < this->word_size; w++) {
		auto left_input_var_idx = this->shift_output_variables.at({idx, w});
		int right_input_var_idx;
		if (idx == 1) {
			// right input is the output of the input node with idx = 0
			right_input_var_idx = this->output_value_variables.at({0, w});
		}
		else {
			// right input is the output of the right shift select mux
			right_input_var_idx = this->shift_select_output_variables.at({idx, scm::right, w});
		}
		for (auto &dir : this->input_directions) {
			auto mux_output_var_idx = this->negate_select_output_variables.at({idx, dir, w});
			if (dir == scm::left) {
				this->create_2x1_mux(right_input_var_idx, left_input_var_idx, select_var_idx, mux_output_var_idx);
				this->constraint_counter++;
			}
			else {
				this->create_2x1_mux(left_input_var_idx, right_input_var_idx, select_var_idx, mux_output_var_idx);
				this->constraint_counter++;
			}
		}
	}
}

void scm::create_xor_constraints(int idx) {
	auto negate_var_idx = this->input_negate_value_variables.at(idx);
	for (int w = 0; w < this->word_size; w++) {
		auto input_var_idx = this->negate_select_output_variables.at({idx, scm::right, w});
		auto output_var_idx = this->xor_output_variables.at({idx, w});
		this->create_2x1_xor(negate_var_idx, input_var_idx, output_var_idx);
		this->constraint_counter++;
	}
}

void scm::create_adder_constraints(int idx) {
	for (int w = 0; w < this->word_size; w++) {
		int c_i;
		if (w == 0) {
			// carry input = input negate value
			c_i = this->input_negate_value_variables.at(idx);
		}
		else {
			// carry input = carry output of last stage
			c_i = this->adder_internal_variables.at({idx, w-1});
		}
		// build sum
		int a = this->negate_select_output_variables.at({idx, scm::left, w});
		int b = this->xor_output_variables.at({idx, w});
		int s = this->output_value_variables.at({idx, w});
		this->create_add_sum(a, b, c_i, s);
		this->constraint_counter++;
		// build carry
		/*
		if (w != this->word_size-1) {
			int c_o = this->adder_internal_variables.at({idx, w});
			this->create_add_carry(a, b, c_i, c_o);
			this->constraint_counter++;
		}
		 */
		int c_o = this->adder_internal_variables.at({idx, w});
		this->create_add_carry(a, b, c_i, c_o);
		this->constraint_counter++;
	}
	// disallow overflows
	this->force_bit(this->adder_internal_variables.at({idx, this->word_size-1}), 0);
	this->constraint_counter++;
}

void scm::create_input_select_limitation_constraints(int idx) {
	auto select_input_word_size = this->ceil_log2(idx);
	int max_representable_input_select = (1 << select_input_word_size) - 1;
	for (auto &dir : this->input_directions) {
		std::vector<int> x(select_input_word_size);
		for (int w = 0; w < select_input_word_size; w++) {
			x[w] = this->input_select_selection_variables.at({idx, dir, w});
		}
		for (int forbidden_number = max_representable_input_select; forbidden_number >= idx; forbidden_number--) {
			this->forbid_number(x, forbidden_number);
			this->constraint_counter++;
		}
	}
}

void scm::create_shift_limitation_constraints(int idx) {
	int max_representable_shift = (1 << this->shift_word_size) - 1;
	std::vector<int> x(this->shift_word_size);
	for (int w = 0; w < this->shift_word_size; w++) {
		x[w] = this->input_shift_value_variables.at({idx, w});
	}
	for (int forbidden_number = max_representable_shift; forbidden_number > this->max_shift; forbidden_number--) {
		this->forbid_number(x, forbidden_number);
		this->constraint_counter++;
	}
}

void scm::get_solution_from_backend() {
	for (int idx = 0; idx <= this->num_adders; idx++) {
		// output_values
		for (int w = 0; w < this->word_size; w++) {
			this->output_values[idx] += (this->get_result_value(this->output_value_variables.at({idx, w})) << w);
		}
		if (idx > 0) {
			int input_node_idx_l = 0;
			int input_node_idx_r = 0;
			if (idx > 1) {
				// input_select
				for (auto &dir : this->input_directions) {
					auto input_select_width = this->ceil_log2(idx);
					for (auto w = 0; w < input_select_width; w++) {
						this->input_select[{idx, dir}] += (this->get_result_value(this->input_select_selection_variables[{idx, dir, w}]) << w);
					}
				}
				// shift_input_select
				this->shift_input_select[idx] = this->get_result_value(this->input_shift_select_variables[idx]);
			}
			// shift_value
			for (auto w = 0; w < this->shift_word_size; w++) {
				this->shift_value[idx] += (this->get_result_value(this->input_shift_value_variables[{idx, w}]) << w);
			}
			// negate_select
			this->negate_select[idx] = this->get_result_value(this->input_negate_select_variables[idx]);
			// subtract
			this->subtract[idx] = this->get_result_value(this->input_negate_value_variables[idx]);
		}
	}
}

void scm::print_solution() {
	if (this->found_solution) {
		std::cout << "Solution for C = " << (this->C << this->output_shift) << " (= " << this->C << " * 2^" << this->output_shift << ")" << std::endl;
		std::cout << "#adders = " << this->num_adders << ", word size = " << this->word_size << std::endl;
		std::cout << "  node #0 = " << this->output_values[0] << std::endl;
		for (auto idx = 1; idx <= this->num_adders; idx++) {
			std::cout << "  node #" << idx << " = " << this->output_values[idx] << std::endl;
			std::cout << "    left input: node " << this->input_select[{idx, scm::left}] << std::endl;
			std::cout << "    right input: node " << this->input_select[{idx, scm::right}] << std::endl;
			std::cout << "    shift input select: " << this->shift_input_select[idx] << std::endl;
			std::cout << "    shift value: " << this->shift_value[idx] << std::endl;
			std::cout << "    negate select: " << this->negate_select[idx] << std::endl;
			std::cout << "    subtract: " << this->subtract[idx] << std::endl;
		}
		if (this->solution_is_valid()) {
			std::cout << "Solution is verified :-)" << std::endl;
		}
		else {
			throw std::runtime_error("Solution is invalid (found bug) :-(");
		}
	}
	else {
		std::cout << "Failed to find solution for C = " << (this->C << this->output_shift) << " (= " << this->C << " * 2^" << this->output_shift << ")" << std::endl;
	}
}

bool scm::solution_is_valid() {
	bool valid = true;
	for (int idx = 1; idx <= this->num_adders; idx++) {
		// verify node inputs
		int input_node_idx_l = 0;
		int input_node_idx_r = 0;
		int actual_input_value_l = 1;
		int actual_input_value_r = 1;
		if (idx > 1) {
			for (auto &dir : this->input_directions) {
				for (auto w = 0; w < this->word_size; w++) {
					this->input_select_mux_output[{idx, dir}] += (
						this->get_result_value(this->input_select_mux_variables[{idx, dir, 0, w}]) << w);
				}
			}
			input_node_idx_l = this->input_select[{idx, scm::left}];
			input_node_idx_r = this->input_select[{idx, scm::right}];
			actual_input_value_l = this->input_select_mux_output[{idx, scm::left}];
			actual_input_value_r = this->input_select_mux_output[{idx, scm::right}];
		}
		auto left_input_value = this->output_values[input_node_idx_l];
		auto right_input_value = this->output_values[input_node_idx_r];
		if (!this->quiet) {
			std::cout << "node #" << idx << " left input" << std::endl;
			std::cout << "  input select = " << input_node_idx_l << std::endl;
			std::cout << "  value = " << this->input_select_mux_output[{idx, scm::left}] << std::endl;
		}
		if (left_input_value != actual_input_value_l) {
			std::cout << "node #" << idx << " has invalid left input" << std::endl;
			std::cout << "  input select = " << input_node_idx_l << std::endl;
			std::cout << "  expected value " << left_input_value << " but got " << this->input_select_mux_output[{idx, scm::left}] << std::endl;
			for (int mux_idx = 0; mux_idx < idx-1; mux_idx++) {
				int mux_output = 0;
				for (auto w = 0; w < this->word_size; w++) {
					mux_output += (this->get_result_value(this->input_select_mux_variables[{idx, scm::left, mux_idx, w}]) << w);
				}
				std::cout << "    mux #" << mux_idx << " output: " << mux_output << std::endl;
			}
			valid = false;
		}
		if (!this->quiet) {
			std::cout << "node #" << idx << " right input" << std::endl;
			std::cout << "  input select = " << input_node_idx_r << std::endl;
			std::cout << "  value = " << this->input_select_mux_output[{idx, scm::right}] << std::endl;
		}
		if (right_input_value != actual_input_value_r) {
			std::cout << "node #" << idx << " has invalid right input" << std::endl;
			std::cout << "  input select = " << input_node_idx_r << std::endl;
			std::cout << "  expected value " << right_input_value << " but got " << this->input_select_mux_output[{idx, scm::right}] << std::endl;
			for (int mux_idx = 0; mux_idx < idx-1; mux_idx++) {
				int mux_output = 0;
				for (auto w = 0; w < this->word_size; w++) {
					mux_output += (this->get_result_value(this->input_select_mux_variables[{idx, scm::right, mux_idx, w}]) << w);
				}
				std::cout << "    mux #" << mux_idx << " output: " << mux_output << std::endl;
			}
			valid = false;
		}
		// verify shift mux outputs
		int shift_mux_output_l = left_input_value;
		int shift_mux_output_r = right_input_value;
		if (idx > 1) {
			if (this->get_result_value(this->input_shift_select_variables[idx]) == 0) {
				shift_mux_output_l = right_input_value;
				shift_mux_output_r = left_input_value;
			}
			std::map<scm::input_direction, int> actual_shift_mux_output;
			for (auto &dir : this->input_directions) {
				for (auto w = 0; w < this->word_size; w++) {
					actual_shift_mux_output[dir] += (this->get_result_value(this->shift_select_output_variables[{idx, dir, w}]) << w);
				}
			}
			if (!this->quiet) {
				std::cout << "node #" << idx << " left shift input select mux output" << std::endl;
				std::cout << "  select = " << this->get_result_value(this->input_shift_select_variables[idx]) << std::endl;
				std::cout << "  output value = " <<  actual_shift_mux_output[scm::left] << std::endl;
			}
			if (shift_mux_output_l != actual_shift_mux_output[scm::left]) {
				std::cout << "node #" << idx << " has invalid left shift input select mux output" << std::endl;
				std::cout << "  select = " << this->get_result_value(this->input_shift_select_variables[idx]) << std::endl;
				std::cout << "  actual value = " <<  actual_shift_mux_output[scm::left] << std::endl;
				std::cout << "  expected value = " << shift_mux_output_l << std::endl;
				valid = false;
			}
			if (!this->quiet) {
				std::cout << "node #" << idx << " right shift input select mux output" << std::endl;
				std::cout << "  select = " << this->get_result_value(this->input_shift_select_variables[idx]) << std::endl;
				std::cout << "  output value = " <<  actual_shift_mux_output[scm::right] << std::endl;
			}
			if (shift_mux_output_r != actual_shift_mux_output[scm::right]) {
				std::cout << "node #" << idx << " has invalid left shift input select mux output" << std::endl;
				std::cout << "  select = " << this->get_result_value(this->input_shift_select_variables[idx]) << std::endl;
				std::cout << "  actual value = " <<  actual_shift_mux_output[scm::right] << std::endl;
				std::cout << "  expected value = " << shift_mux_output_r << std::endl;
				valid = false;
			}
		}
		// verify shifter output
		int expected_shift_output = (shift_mux_output_l << this->shift_value[idx]) % (1 << this->word_size);
		int actual_shift_output = 0;
		for (int w = 0; w < this->word_size; w++) {
			actual_shift_output += (this->get_result_value(this->shift_output_variables[{idx, w}]) << w);
		}
		if (!this->quiet) {
			std::cout << "node #" << idx << " shift output" << std::endl;
			std::cout << "  input value = " << shift_mux_output_l << std::endl;
			std::cout << "  shift value = " << this->shift_value[idx] << std::endl;
			std::cout << "  output value = " << actual_shift_output << std::endl;
		}
		if (expected_shift_output != actual_shift_output) {
			std::cout << "node #" << idx << " has invalid shift output" << std::endl;
			std::cout << "  input value = " << shift_mux_output_l << std::endl;
			std::cout << "  shift value = " << this->shift_value[idx] << std::endl;
			std::cout << "  expected output value = " << expected_shift_output << std::endl;
			std::cout << "  actual output value = " << actual_shift_output << std::endl;
			valid = false;
		}
		// verify negate mux outputs
		int negate_mux_output_l = actual_shift_output;
		int negate_mux_output_r = shift_mux_output_r;
		if (this->get_result_value(this->input_negate_select_variables[idx]) == 0) {
			negate_mux_output_l = shift_mux_output_r;
			negate_mux_output_r = actual_shift_output;
		}
		std::map<scm::input_direction, int> actual_negate_mux_output;
		for (auto &dir : this->input_directions) {
			for (auto w = 0; w < this->word_size; w++) {
				actual_negate_mux_output[dir] += (this->get_result_value(this->negate_select_output_variables[{idx, dir, w}]) << w);
			}
		}
		if (!this->quiet) {
			std::cout << "node #" << idx << " left negate select mux output" << std::endl;
			std::cout << "  select = " << this->get_result_value(this->input_negate_select_variables[idx]) << std::endl;
			std::cout << "  output value = " <<  actual_negate_mux_output[scm::left] << std::endl;
		}
		if (negate_mux_output_l != actual_negate_mux_output[scm::left]) {
			std::cout << "node #" << idx << " has invalid left negate select mux output" << std::endl;
			std::cout << "  select = " << this->get_result_value(this->input_negate_select_variables[idx]) << std::endl;
			std::cout << "  actual value = " <<  actual_negate_mux_output[scm::left] << std::endl;
			std::cout << "  expected value = " << negate_mux_output_l << std::endl;
			valid = false;
		}
		if (!this->quiet) {
			std::cout << "node #" << idx << " right negate select mux output" << std::endl;
			std::cout << "  select = " << this->get_result_value(this->input_negate_select_variables[idx]) << std::endl;
			std::cout << "  output value = " <<  actual_negate_mux_output[scm::right] << std::endl;
		}
		if (negate_mux_output_r != actual_negate_mux_output[scm::right]) {
			std::cout << "node #" << idx << " has invalid right negate select mux output" << std::endl;
			std::cout << "  select = " << this->get_result_value(this->input_negate_select_variables[idx]) << std::endl;
			std::cout << "  actual value = " <<  actual_negate_mux_output[scm::right] << std::endl;
			std::cout << "  expected value = " << negate_mux_output_r << std::endl;
			valid = false;
		}
		// verify xor output
		int sub = this->get_result_value(this->input_negate_value_variables[idx]);
		int expected_xor_output = sub==1?(~negate_mux_output_r) & ((1 << this->word_size) - 1):negate_mux_output_r;
		int actual_xor_output = 0;
		for (int w = 0; w < this->word_size; w++) {
			actual_xor_output += (this->get_result_value(this->xor_output_variables[{idx, w}]) << w);
		}
		if (!this->quiet) {
			std::cout << "node #" << idx << " xor output" << std::endl;
			std::cout << "  sub = " << sub << std::endl;
			std::cout << "  input value = " << negate_mux_output_r << std::endl;
			std::cout << "  output value = " <<  actual_xor_output << std::endl;
		}
		if (expected_xor_output != actual_xor_output) {
			std::cout << "node #" << idx << " has invalid xor output" << std::endl;
			std::cout << "  sub = " << sub << std::endl;
			std::cout << "  input value = " << negate_mux_output_r << std::endl;
			std::cout << "  actual output value = " <<  actual_xor_output << std::endl;
			std::cout << "  expected output value = " << expected_xor_output << std::endl;
			valid = false;
		}
		// verify node/adder output
		int expected_adder_output = (negate_mux_output_l + actual_xor_output + sub) & ((1 << this->word_size) - 1);
		int actual_adder_output = 0;
		for (int w = 0; w < this->word_size; w++) {
			actual_adder_output += (this->get_result_value(this->output_value_variables[{idx, w}]) << w);
		}
		if (!this->quiet) {
			std::cout << "node #" << idx << " output value" << std::endl;
			std::cout << "  sub = " << sub << std::endl;
			std::cout << "  left input value = " << negate_mux_output_l << std::endl;
			std::cout << "  right input value = " << actual_xor_output << std::endl;
			std::cout << "  actual output value = " << actual_adder_output << std::endl;
		}
		if (expected_adder_output != actual_adder_output) {
			std::cout << "node #" << idx << " has invalid output value" << std::endl;
			std::cout << "  sub = " << sub << std::endl;
			std::cout << "  left input value = " << negate_mux_output_l << std::endl;
			std::cout << "  right input value = " << actual_xor_output << std::endl;
			std::cout << "  expected output value = " << expected_adder_output << std::endl;
			std::cout << "  actual output value = " << actual_adder_output << std::endl;
		}
	}
	return valid;
}