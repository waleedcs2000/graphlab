/*
 * kbp.cpp
 *
 * This file contains codes for kernel
 * belief propagation in a pairwise markov random field to
 * estimate depth from features from still images;
 *
 *  Created on: Oct 10, 2010
 *      Author: lesong
 */

// INCLUDES ===================================================================>

// Including Standard Libraries
#include <iostream>
#include <map>

#include <graphlab.hpp>
#include "image.hpp"

// include itpp
#include <itpp/itstat.h>
#include <itpp/itbase.h>

// Include the macro for the for each operation
#include <graphlab/macros_def.hpp>

using namespace itpp;
using namespace std;

//// STRUCTS (Edge and Vertex data) =============================================>
//struct edge_data: public graphlab::unsupported_serialize {
//  vec message;
//  vec old_message;
//  // vec lfeat_message;
//  int update_count;
//}; // End of edge data
//
//struct vertex_data: public graphlab::unsupported_serialize {
//  vec belief;
//  size_t row, col;
//};
//
//typedef graphlab::graph<vertex_data, edge_data> graph_type;
//typedef graphlab::types<graph_type> gl_types;

mat* iCfyfy;
mat* Cfyx;
mat* iCxx;
vec* fmu;
mat** Uttp;
mat** Uttp0;
mat** Utpt;
mat** Utpt0;

ivec ano;
mat cfybasis;
mat ftesty;
mat testy;

mat coord;
mat feat;
ivec atype;
ivec chainlen;
ivec testlen;

// MAIN =======================================================================>
int main(int argc, char** argv) {
  std::cout << "kernel bp with denoising data" << std::endl;

  // set the global logger
  global_logger().set_log_level(LOG_WARNING);
  global_logger().set_log_to_console(true);

  std::string prefix(argv[1]);
  std::string fname = prefix + "_common.it";

  std::cout << "file to load: " << fname << std::endl;
  it_ifile common(fname.c_str());
  common >> Name("ano") >> ano;
  common >> Name("cfybasis") >> cfybasis;
  common >> Name("ftesty") >> ftesty;
  common >> Name("testy") >> testy;
  common >> Name("testlen") >> testlen;

  iCfyfy = new mat[ano[0]];
  Cfyx = new mat[ano[0]];
  iCxx = new mat[ano[0]];
  fmu = new vec[ano[0]];

  for (size_t i = 0; i < (size_t)ano[0]; i++) {
	  std::stringstream ss;
	  ss << i + 1;
	  fname = prefix + "_node" + ss.str() + ".it";
	  it_ifile node(fname.c_str());

	  std::cout << "file to load: " << fname << std::endl;
	  node >> Name("iCfyfyi") >> iCfyfy[i];
	  node >> Name("Cfyxi") >> Cfyx[i];
	  node >> Name("iCxxi") >> iCxx[i];
	  node >> Name("fmui") >> fmu[i];

	  node.close();
  }

  Uttp = new mat*[ano[0]];
  Uttp0 = new mat*[ano[0]];
  Utpt = new mat*[ano[0]];
  Utpt0 = new mat*[ano[0]];
  for (size_t i = 0; i < (size_t)ano[0]; i++) {
	  std::stringstream ssi;
	  ssi << i + 1;

	  Uttp[i] = new mat[ano[0]];
	  Uttp0[i] = new mat[ano[0]];
	  Utpt[i] = new mat[ano[0]];
	  Utpt0[i] = new mat[ano[0]];
	  for (size_t j = 0; j < (size_t)ano[0]; j++) {
		  std::stringstream ssj;
		  ssj << j + 1;
		  fname = prefix + "_edge" + ssi.str() + "_" + ssj.str() + ".it";
		  it_ifile edge(fname.c_str());

		  std::cout << "file to load: " << fname << std::endl;

		  edge >> Name("Uttpij") >> Uttp[i][j];
		  edge >> Name("Uttp0ij") >> Uttp0[i][j];
		  edge >> Name("Utptij") >> Utpt[i][j];
		  edge >> Name("Utpt0ij") >> Utpt0[i][j];

		  edge.close();
	  }
  }

  std::cout << "we are here " << argv[2] << std::endl;

  std::string prefix1(argv[2]);
  vec errmat(testlen[0]);
  for (size_t t = 0; t < (size_t)testlen[0]; t++) {
	  std::stringstream ss;
	  ss << t + 1;
	  std::string fname1 = prefix1 + ss.str() + ".it";

	  std::cout << "Processing test file: " << t+1 << " of " << testlen[0] << std::endl;

	  it_ifile test(fname1.c_str());
	  test >> Name("coord") >> coord;
	  test >> Name("feat") >> feat;
	  test >> Name("atype") >> atype;
	  test >> Name("chainlen") >> chainlen;
	  test.close();

	  vec* msg_forward = new vec[chainlen(0)];
	  vec* msg_backward = new vec[chainlen(0)];
	  vec* belief = new vec[chainlen(0)];
	  vec* likelihood = new vec[chainlen(0)];

	  // forward message passing;
	  for (size_t j = 0; j < (size_t)chainlen(0)-1; j++) {
//		  std::cout << "forward message " << j << std::endl;

		  likelihood[j] = iCfyfy[atype(j)-1] * Cfyx[atype(j)-1] * iCxx[atype(j)-1] * feat.get_col(j);
		  belief[j] = elem_mult(ftesty.transpose() * likelihood[j], ftesty.transpose() * fmu[atype(j)-1]);

		  if (j < 1) {
			  msg_forward[j] = Utpt0[atype(j)-1][atype(j+1)-1] * likelihood[j];
		  } else {
			  msg_forward[j] = Utpt[atype(j)-1][atype(j+1)-1] * elem_mult(cfybasis.transpose() * msg_forward[j-1], cfybasis.transpose() * likelihood[j]);
			  elem_mult_inplace(ftesty.transpose() * msg_forward[j-1], belief[j]);
		  }
	  }
	  likelihood[chainlen(0)-1] = iCfyfy[atype(chainlen(0)-1)-1] * Cfyx[atype(chainlen(0)-1)-1] * iCxx[atype(chainlen(0)-1)-1] * feat.get_col(chainlen(0)-1);
	  belief[chainlen(0)-1] = elem_mult(ftesty.transpose() * likelihood[chainlen(0)-1], elem_mult(ftesty.transpose() * fmu[atype(chainlen(0)-1)-1], ftesty.transpose() * msg_forward[chainlen(0)-2]));
    graphlab::timer ti;
    ti.start();
	  // backward message passing;
	  for (size_t j = (size_t)chainlen(0)-1; j > 0; j--) {
//		  std::cout << "backward message " << j << std::endl;

		  if (j > (size_t)chainlen(0) - 2) {
			  msg_backward[j] = Uttp0[atype(j-1)-1][atype(j)-1] * likelihood[j];
		  } else {
			  msg_backward[j] = Uttp[atype(j-1)-1][atype(j)-1] * elem_mult(cfybasis.transpose() * msg_backward[j+1], cfybasis.transpose() * likelihood[j]);
			  elem_mult_inplace(ftesty.transpose() * msg_backward[j+1], belief[j]);
		  }
	  }
	  elem_mult_inplace(ftesty.transpose() * msg_backward[1], belief[0]);

	  std::cout << "Computing alignment" << std::endl;
	  mat coord_pred(3, chainlen(0));
	  for (size_t i = 0; i < (size_t)chainlen(0); i++) {
		  coord_pred.set_col(i, testy.get_col(max_index(belief[i])));
	  }
	  vec err = sum(elem_mult(coord, coord_pred), 1);
	  errmat(t) = mean(err);
	  std::cout << "Average alignment: " << errmat(t) << " and so far " << mean(errmat(0,t)) << std::endl;
    
      ofstream fout;
      fout.open("log_protein.txt", ios::app);
      std::string f = fname1.rfind("/") == std::string::npos ?
                                        fname1:
                                        fname1.substr(fname1.rfind("/")+1);
      
      fout << "kbp\t" << fname1 << "\t" << 0 << "\t" << 0 << "\t" << errmat(t) << "\t"
      << ti.current_time() << std::endl;
      fout.close();
      
    
	  delete [] likelihood;
	  delete [] belief;
	  delete [] msg_backward;
	  delete [] msg_forward;
  }

  std::cout << "Final average aligment: " << mean(errmat) << std::endl;

  for (size_t i = (size_t)ano[0]-1; i >= 0; i--) {
	  delete [] Utpt0[i];
	  delete [] Utpt[i];
	  delete [] Uttp0[i];
	  delete [] Uttp[i];
  }
  delete [] Uttp;
  delete [] Uttp0;
  delete [] Utpt;
  delete [] Utpt0;


  std::cout << "Done!" << std::endl;

  return EXIT_SUCCESS;
} // End of main
