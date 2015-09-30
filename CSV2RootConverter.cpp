// CSV2RootConverter.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
/*
#include "stdio.h"
#include "time.h"
*/

#include <TFile.h>
#include <TTree.h>

using namespace std;


bool FileNameCheck(const string inputfilename, const string fileextension)
{
	string filename = inputfilename;
	
	string extlower = fileextension;
	transform(extlower.begin(), extlower.end(), extlower.begin(), tolower);
	
	string extupper = fileextension;
	transform(extupper.begin(), extupper.end(), extupper.begin(), toupper);
	
	std::size_t found = filename.find_last_of(".");
	
	if (found != string::npos) {
		string extension = "";
		
		if (found < filename.length()-1) extension = filename.substr(found+1,filename.length()-(found+1));
		
		if ((extension != extlower) && (extension != extupper)) {
			cerr << "ERROR: '" << inputfilename << "' has wrong fileextension. Expected '." << fileextension << "'" << endl;
			return false;
		}
	}
	
	return true;
}

void PrintHelp(char argv[100]) {
	cerr << "Usage:" << endl;
		cerr << "\t" << argv << " (OPTIONS) OUTPUTFILE INPUTFILE1 (INPUTFILE2 ...)" << endl;
		cerr << endl;
		cerr << "Options:" << endl;
		cerr << "\t--help\t\t\tPrint this message" << endl;
		cerr << "\t--delim / -d DELIMITER\tSet input seperator (default: '\\t' (Tab), specify within double quotes)" << endl;
		cerr << endl;
}

int main(int argc, char* argv[]){
	cerr << endl;
	cerr << "This is the CSV to Root converter!" << endl;
	cerr << "Version 0.1.2, 2015-09-30" << endl;
	cerr << endl;
	cerr << "Developed by" << endl;
	cerr << "\tInfineon Technologies AG, Warstein" << endl;
	cerr << "\tDr. Georg Troska & Dr. Till Neddermann" << endl;
	cerr << endl;

	int minargs = 2;
	int optionargs = 0;

	if (argc < minargs+1) {
		PrintHelp(argv[0]);

		return -1;
	} else {
		//string outfilename(argv[1]);
		string outfilename = "";
		vector<string> inputfilenames;
		string inputfilename = "";
		string delimiter = "\t";

		for (int i=1; i<argc; i++) {
			std::string optionstring(argv[i]);

			//cerr << "Argument " << i << ": '" << optionstring << "'" << endl;
			if (optionstring == "--help") {
				PrintHelp(argv[0]);

				return -2;
			}
			else if (optionstring == "--delim" || optionstring == "-d") {
				i++;
				delimiter = string(argv[i]);
				//cerr << "Argument " << i << ": '" << delimiter << "'" << endl;
				optionargs += 2;
			}
			else {
				if (outfilename != "")
					inputfilenames.push_back(string(argv[i]));
				else {
					outfilename = string(argv[i]);
				}
			}
		}

		if (outfilename == "") {
			cerr << endl << "ERROR: No output file provided. Exiting..." << endl << endl;
			return -2;
		}
		else if (inputfilenames.size() == 0) {
			cerr << endl << "ERROR: No input file provided. Exiting..." << endl << endl;
			return -2;
		}

		//////

		if (!FileNameCheck(outfilename, "root")) {
			cerr << endl << "ERROR: Output file of wrong type. '.root' required. Exiting..." << endl << endl;
			return -2;
		}

		TFile *outfile = new TFile(outfilename.c_str(),"RECREATE");

		// TODO: Check if file is open
		
		TTree *tree = new TTree("tree","tree");

		char cline[4096];
		std::string sline;
		int lineNumber = 0;
		std::string name[1000];
		int kind[1000]; // 0: none, 1: string, 2: int, 3: float

		fprintf(stderr, "Input files:\n");

		int varCounter = 0;
		int varCounter_old = 0;

		int entries_total = 0;

		//infile.getline(cline,4095);
		for (unsigned int ifileindex=0; ifileindex<inputfilenames.size(); ifileindex++) {

			inputfilename = inputfilenames[ifileindex];
			
			fprintf(stderr, "\t%s\n", inputfilename.c_str());

			ifstream infile (inputfilename.c_str());
			if (!infile.is_open()) {
				cerr << "ERROR: Unable to open input file '" << inputfilename << "'!" << endl;
			} else {
				
				// Store number of columns from first input file for security check
				if (ifileindex ==1)	varCounter_old = varCounter;
				lineNumber = 0;

				while (!infile.eof()) { // row by row
					infile.getline(cline,4095);
					sline.assign(cline);
					//cout << sline << endl;
					if (sline.length() < 2) continue; // to avoid problems with empty lines at the end
	
					unsigned int tabStart = 0;
					unsigned int tabStop = 0;
					varCounter = 0;
	
	
					std::string varStr[1000];
					int varInt[1000];
					double varFlt[1000];
	
					for (int i = 0; i < 1000; i++) {
						varStr[i] = "";
						varInt[i] = 0;
						varFlt[i] = 0.;
					}
	
	
					std::string var = "";
	
					//Cut the string-line into var by var
					while (tabStop < sline.length() -1) { // one col in one row
						tabStop = sline.find_first_of(delimiter,tabStart);
						if (tabStop < 0) {
								tabStop = sline.length();
							}
						var.assign(sline,tabStart,tabStop-tabStart);
						//cout << "var: " << var << endl;
						if (lineNumber == 0) {
							if (ifileindex == 0) name[varCounter] = var;
							else if (name[varCounter] != var) {
								cerr << endl << "ERROR: Input '" << inputfilename << "': Column head '" << var << "' in column " << varCounter << " differs from '" << name[varCounter] << "' (first file)!" << endl << endl;
								outfile->Close();
								infile.close();
								return -1;
							}
							//cout << "name at " << varCounter << " is " << var << endl;	
						}
						else if (lineNumber == 1) {
							int mykind(0);

							std::transform(var.begin(), var.end(), var.begin(),::toupper);

							if (name[varCounter].length() < 1) {
								mykind = 0;
								if (ifileindex == 0) cout << "\t\tColumn " << varCounter << " will be ignored, no name is given" << endl;
							} else if (strcmp(var.c_str(),"STRING")==0) {
								mykind = 1;
								if (ifileindex == 0) {
									tree->Branch(name[varCounter].c_str(), &varStr[varCounter]); 
									cout << "\t\tColumn " << varCounter << " named " << name[varCounter] << " is STRING" << endl;
								}
							} else if (strcmp(var.c_str(),"INT")==0 || strcmp(var.c_str(),"DINT")==0 || strcmp(var.c_str(),"BOOL")==0) {
								mykind = 2;
								if (ifileindex == 0) {
									tree->Branch(name[varCounter].c_str(), &varInt[varCounter]);
									cout << "\t\tColumn " << varCounter << " named " << name[varCounter] << " is INT" << endl;
								}
							} else if (strcmp(var.c_str(),"REAL")==0) {
								mykind = 3;
								if (ifileindex == 0) {
									tree->Branch(name[varCounter].c_str(), &varFlt[varCounter]);
									cout << "\t\tColumn " << varCounter << " named " << name[varCounter] << " is REAL" << endl;
								}
							} else if (var.length() < 1) {
								mykind = 0;
								if (ifileindex == 0) cout << "\t\tColumn " << varCounter << " named " << name[varCounter] << " will be ignored, no type is given" << endl;
							}

							if (ifileindex == 0) kind[varCounter] = mykind;
							else if (kind[varCounter] != mykind) {
								cerr << endl << "ERROR: Input '" << inputfilename << "': Column type '" << mykind << "' in column " << varCounter << " differs from '" << kind[varCounter] << "' (first file)!" << endl << endl;
								outfile->Close();
								infile.close();
								return -1;
							}
						} else if (lineNumber > 1) {
							if (kind[varCounter] == 1) {
								varStr[varCounter] = var;
								//cout << "var is: " << var << endl;
							} else if (kind[varCounter] == 2) {
								if (strcmp(var.c_str(),"FALSCH")==0 || strcmp(var.c_str(),"FALSE")==0) {
									varInt[varCounter] = 0;
								} else if (strcmp(var.c_str(),"WAHR")== 0 || strcmp(var.c_str(),"TRUE")==0) {
									varInt[varCounter] = 1;
								} else {
									varInt[varCounter] = atoi(var.c_str());
								}
							} else if (kind[varCounter] == 3) {
								varFlt[varCounter] = atof(var.c_str());
							}
						}
		
						tabStart = tabStop +1;
						varCounter++;
		
					}
					if (lineNumber > 1) {
						//cout << "filling...";
						tree->Fill();
						//cout << "   ...filled " << lineNumber<< endl;
					}
	
					if (lineNumber == 0 && ifileindex > 0 && varCounter != varCounter_old) {
							// Abort if number of columns from second (...) input file differs from first one
							cerr  << endl << "ERROR: Number of columns in '" << inputfilename << "' differs from first file! Aborting ..." << endl << endl;
							outfile->Close();
							infile.close();
							return -1;
					}

					// Put this line in and it will work - why????
					//else if (lineNumber == 1) tree->Scan("b");
					lineNumber++;
				}

				outfile->Write(0,TObject::kOverwrite);
				infile.close();
				cout << "\t\t" << lineNumber -2 << " entries written from file '" << inputfilename << "' with " << varCounter << " columns " << endl;

				entries_total += lineNumber -2;
			}
		}
		cout << endl << entries_total << " entries written to file '" << outfilename << "'." << endl << endl;
		outfile->Close();
	}
	return 0;
}

