#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

#include "../include/func.h"
#include "../etana/proof.h"

	extern "C" {
			#include "../include/HTS_engine.h"
		}
#define BOM8A 0xEF
#define BOM8B 0xBB
#define BOM8C 0xBF
		

CLinguistic Linguistic;
CDisambiguator Disambiguator;

typedef unsigned char uchar;

wchar_t *UTF8_to_WChar(const char *string){
	long b=0,
		c=0;
	//if ((uchar)string[0]==BOM8A && (uchar)string[1]==BOM8B && (uchar)string[2]==BOM8C) string+=3;
	for (const char *a=string;*a;a++)
		if (((uchar)*a)<128 || (*a&192)==192)
			c++;
	wchar_t *res=new wchar_t[c+1];

	res[c]=0;
	for (uchar *a=(uchar*)string;*a;a++){
		if (!(*a&128))
			res[b]=*a;
		else if ((*a&192)==128)
			continue;
		else if ((*a&224)==192)
			res[b]=((*a&31)<<6)|a[1]&63;
		else if ((*a&240)==224)
			res[b]=((*a&15)<<12)|((a[1]&63)<<6)|a[2]&63;
		else if ((*a&248)==240){
			res[b]='?';
		}
		b++;
	}
	return res;
}

void ReadUTF8Text ( CFSWString &text, const char *fn) {

		std::ifstream fs;
		fs.open (fn, std::ios::binary);
		if (fs.fail()) {
			std::cerr << "Ei leia sisendteksti!\n";
			exit(1);
		}
		fs.seekg(0, std::ios::end);
		size_t i = fs.tellg();
		char* buf = new char[i];
		fs.seekg(0, std::ios::beg);
		fs.read (buf, i);
		fs.close();

		wchar_t* w_temp;
		w_temp = UTF8_to_WChar(buf);
		delete [] buf;
		text = w_temp;
}



int PrintUsage() {
	wprintf (L"\nKautamine\n");
	wprintf (L"\nbin/synthts_et -lex dct/et.dct -lexd dct/et3.dct -m htsvoices/eki_et_eva001.htsvoice -o tulemus_e001.wav -f sisend.txt\n");
	return 0;
}


char *convert_vec(const std::string & s) {
   char *pc = new char[s.size()+1];
   strcpy(pc, s.c_str());
   return pc; 
	}


void fill_char_vector (std::vector<std::string>& v, std::vector<char*>& vc) {
		std::transform(v.begin(), v.end(), std::back_inserter(vc), convert_vec);
	}


void clean_char_vector (std::vector<char*>& vc) {
	    for ( size_t x = 0 ; x < vc.size() ; x++ ) 
       	delete [] vc[x];	
	}


std::string to_stdstring(CFSWString s) {
	std::string res = "";
		for (INTPTR i = 0; i < s.GetLength(); i ++)
			res += s.GetAt(i);
	return res;
}


std::vector<std::string> to_vector (CFSArray<CFSWString> arr) {
	std::vector<std::string> v;
	for (INTPTR i = 0; i < arr.GetSize(); i++) 
		v.push_back(to_stdstring(arr[i]));
	return v;
}

void cfileexists(const char * filename) {
    FILE *file;
    if (file = fopen(filename, "r")){
        fclose(file);
        remove(filename);
    }
	}


int main(int argc, char **argv)
{
  size_t num_voices;
  char **fn_voices;
	char* in_fname;
	char* output_fname;
	const char *temp_fname = "temp";
	FILE * tmpfp;
	FILE * outfp;
	
	CFSAString LexFileName, LexDFileName;
	HTS_Engine engine;
	double speed = 1.0;
	size_t fr = 48000;
	size_t fp = 240;
	float alpha = 0.55;
	float beta = 0.0;
	float ht = 2.0;
	float th = 0.5;
	float gvw1 = 1.2;
	float gvw2 = 1.2;

	FSCInit();	

fn_voices = (char **) malloc(argc * sizeof(char *));	
	
		for (int i=0; i<argc; i++) {
			if (CFSAString("-lex")==argv[i]) {
				if (i+1<argc) {
					LexFileName=argv[++i];
				} else {
					return PrintUsage();
				}
			}	
			if (CFSAString("-lexd")==argv[i]) {
				if (i+1<argc) {
					LexDFileName=argv[++i];
				} else {
					return PrintUsage();
				}
			}	
			if (CFSAString("-m")==argv[i]) {
				if (i+1<argc) {
						fn_voices[0] = argv[i+1];
            }
            else 
            	{ 
                std::cerr << "puudub *.htsvoice fail" << std::endl;
                exit(0);
            	}              
            }
			if (CFSAString("-o")==argv[i]) {
						if (i+1<argc) {
                output_fname = argv[i+1];
                cfileexists(output_fname);                
            }
            else 
            	{ 
                std::cerr << "puudb väljundfaili nimi" << std::endl;
                exit(0);
            	}              
            }
		
			if (CFSAString("-f")==argv[i]) {
						if (i+1<argc) {
                in_fname = argv[i+1];               
            }
            else 
            	{ 
                std::cerr << "puudb väljundfaili nimi" << std::endl;
                exit(0);
            	}              
            }

		}
	
	
	Linguistic.Open(LexFileName);
	Disambiguator.Open(LexDFileName);
  CFSWString text;
  ReadUTF8Text(text, in_fname);
  text.Delete(text.GetLength()-1, 1);
	HTS_Engine_initialize(&engine);


	if (HTS_Engine_load(&engine, fn_voices, 1) != TRUE) {
			fprintf(stderr, "Viga: puudub *.htsvoice. %i %p\n", num_voices, fn_voices[0]);
			free(fn_voices);
			HTS_Engine_clear(&engine);
			exit(1);
		}
	free(fn_voices);

	HTS_Engine_set_sampling_frequency(&engine, (size_t) fr );
	HTS_Engine_set_phoneme_alignment_flag(&engine, FALSE);
	HTS_Engine_set_fperiod(&engine, (size_t) fp );
	HTS_Engine_set_alpha(&engine, alpha);
	HTS_Engine_set_beta(&engine, beta);
	HTS_Engine_set_speed(&engine, speed);
	HTS_Engine_add_half_tone(&engine, ht);
	HTS_Engine_set_msd_threshold(&engine, 1, th);
	HTS_Engine_set_gv_weight(&engine, 0, gvw1);
	HTS_Engine_set_gv_weight(&engine, 1, gvw2);
	
  text = DealWithText(text);
  
  CFSArray<CFSWString> res =  do_utterances(text);  
  int data_size = 0;
  tmpfp = fopen(temp_fname, "ab");
  
 	for (INTPTR i = 0; i < res.GetSize(); i++) {
 	 	
 	 	CFSArray<CFSWString> label =	do_all (res[i]);
 	 	
 	 	std::vector<std::string> v;
 	 	v = to_vector(label);

 		std::vector<char*>  vc;
		fill_char_vector (v, vc);

		size_t n_lines = vc.size();
			
			if (HTS_Engine_synthesize_from_strings(&engine, &vc[0], n_lines) != TRUE) {
				fprintf(stderr, "Viga: syntees ebaonnestus.\n");
				HTS_Engine_clear(&engine);
				exit(1);
		}

		clean_char_vector(vc);
		data_size += HTS_Engine_engine_speech_size(&engine);	
		HTS_Engine_save_generated_speech(&engine, tmpfp);

		HTS_Engine_refresh(&engine);
 
  } //synth loop
  fclose(tmpfp);
  
	tmpfp = fopen(temp_fname, "rb");
	outfp = fopen(output_fname, "wb");
	HTS_Engine_write_header(&engine, outfp, data_size); 

	size_t n, m;
	unsigned char buff[2];
	do {
    	n = fread(buff, 1, sizeof buff, tmpfp);
    	if (n) m = fwrite(buff, 1, n, outfp);
    		else   m = 0;
			} while ((n > 0) && (n == m));
			if (m) perror("copy");

	fclose(tmpfp);
	fclose(outfp);
  remove(temp_fname);
  HTS_Engine_clear(&engine);
	Linguistic.Close();
	FSCTerminate();
  return 0;

}















