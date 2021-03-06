/*
Galley script to take all the flux read, ppfx weighted art root files
and weights the flux based on the ppfx weights and importance weights etc.

This specific script overwrites the window flux method to compare the flux at
the detector (smeared). It also breaks the flux down by parent to give the
oppertunuty to investigate the flux by parent.

All the window method and detector smeared weights are preserved. 

This script still hardcodes the POT. It is locked away in the subrun
sumdata::POTsummary dataproduct which I have no idea how to access in 
gallery...

* Authors: J. Zennamo, A. Mastbaum, K. Mistry
*/

// Helper functions and some histogram defintions are found in functions_makehist.h
#include "functions_makehist_v2.h"

using namespace art;
using namespace std;
using namespace std::chrono;

// USAGE:
// makehist_parent <detector_type> <root_files>
// <detector_type>: uboone OR nova DEFAULT IS uboone
// Make sure that the root files contain the correct flux reader module ran over the right geometry
//___________________________________________________________________________
int main(int argc, char** argv) {

    auto start = high_resolution_clock::now(); // Start time of script

    InputTag mctruths_tag { "flux" };
    // InputTag  evtwght_tag { "eventweight" };

    bool extra_genie = false;

    // Variable to control what set we are in
    int set = 1;

    // Beamline and CV
    InputTag eventweight_tag_00("eventweight","","EventWeightSept24");
    InputTag eventweight_tag_01("eventweight","","EventWeightSept24ExtraGENIE1");
    
    // Set 2
    // set = 2;
    // std::cout << "Running Set 2"<< std::endl;
    // InputTag eventweight_tag_00("eventweight","","EventWeightSept24ExtraGENIE2");
    // InputTag eventweight_tag_01("eventweight","","EventWeightSept24ExtraGENIE3");
    
    // Set 3
    // set = 3;
    // std::cout << "Running Set 3"<< std::endl;
    // InputTag eventweight_tag_00("eventweight","","EventWeightSept24ExtraGENIE4");
    // InputTag eventweight_tag_01("eventweight","","EventWeightSept24ExtraGENIE5");
    
    std::vector<std::string> labels = {"ppfx_mippk_PPFXMIPPKaon",
                                       "ppfx_mipppi_PPFXMIPPPion",
                                       "ppfx_other_PPFXOther",
                                       "ppfx_targatt_PPFXTargAtten",
                                       "ppfx_think_PPFXThinKaon",
                                       "ppfx_thinmes_PPFXThinMeson",
                                       "ppfx_thinnpi_PPFXThinNeutronPion",
                                       "ppfx_thinna_PPFXThinNucA",
                                       "ppfx_thinn_PPFXThinNuc",
                                       "ppfx_thinpi_PPFXThinPion",
                                       "ppfx_totabs_PPFXTotAbsorp",
                                       "ppfx_ms_UBPPFX"};

    double totalPOT{0};
    int n_universes = 100; // By default set to 100 unless we override this

    std::vector<std::string> badfiles;
    std::vector<std::string> filename;

    // systematic - universe 
    std::vector< std::vector< double > > Weights;  
    std::vector< std::vector< float > > Weights_float;  
    float theta; 
    float Enu;
    float Pmom_dk;
    float Pmom_tg;
    float cv_weight;
    float dk2nu_weight;
    int    tree_nu_pdg; // True neutrino pdg
    int    par_pdg; // Tree nuetrino parent pdg
    float decay_zpos; // Parent decay position
    float imp_weight; // Importance weight

    std::vector<std::string> flav = { "numu", "nue", "numubar", "nuebar" };

    std::vector<double> temp, temp2; // For the bins

    // Histograms for each flavor
    std::vector<TH1D*> Enu_CV_Window;	
    std::vector<TH1D*> Enu_CV_AV_TPC;	
    std::vector<TH1D*> Enu_UW_Window;	
    std::vector<TH1D*> Enu_UW_AV_TPC;	

    std::vector<TH1D*> Th_CV_AV_TPC;	
    std::vector<TH1D*> Th_UW_AV_TPC;	
    std::vector<TH1D*> Th_CV_AV_TPC_DIF;	
    std::vector<TH1D*> Th_UW_AV_TPC_DIF;	

    std::vector<TH1D*> Baseline_CV_AV_TPC;
    std::vector<TH1D*> Baseline_CV_AV_TPC_DIF;
    std::vector<TH2D*> Enu_Baseline_CV_AV_TPC_2D;

    // 5Mev Bins
    std::vector<TH1D*> Enu_CV_Window_5MeV_bin;	
    std::vector<TH1D*> Enu_CV_AV_TPC_5MeV_bin;	
    std::vector<TH1D*> Enu_UW_Window_5MeV_bin;	
    std::vector<TH1D*> Enu_UW_AV_TPC_5MeV_bin;	

    std::vector<TH1D*> Enu_CV_AV_TPC_5MeV_bin_DIF;	
    std::vector<TH1D*> Enu_UW_AV_TPC_5MeV_bin_DIF;	

    // Detector intersection window method
    std::vector<TH1D*> Enu_CV_Window_5MeV_bin_intersect;	

    // Flux by Parent
    std::vector<std::string> parent = {"PI_Plus", "PI_Minus", "Mu_Plus", "Mu_Minus", "Kaon_Plus", "Kaon_Minus" , "K0L"};
    std::vector<std::vector<TH1D*> > Enu_Parent_AV_TPC;		// Flux by Parent
    std::vector<std::vector<TH1D*> > Th_Parent_AV_TPC; 		// Flux by parent in theta
    std::vector<std::vector<TH1D*> > zpos_Parent_AV_TPC; 	// Flux by parent in z Pos at decay (production is unimplemented for now)
    std::vector<std::vector<TH1D*> > dk_Parent_mom; 		// Decay momentum by parent
    std::vector<std::vector<TH1D*> > impwght_Parent; 		// Importance weight by parent
    std::vector<std::vector<TH1D*> > Prod_energy_Parent; 	// Production energy by parent
    std::vector<std::vector<TH1D*> > Targ_mom_Parent; 	    // Momentum by parent as it leaves the target
    std::vector<std::vector<TH1D*> > DAR_Enu_Parent; 	    // Energy spectrum of decay at rest particles
    std::vector<std::vector<TH1D*> > DAR_Th_Parent; 	    // Energy spectrum of decay at rest particles
    std::vector<std::vector<TH1D*> > DIF_Enu_Parent; 	    // Energy spectrum of decay in flight particles
    std::vector<std::vector<TH1D*> > DIF_Th_Parent; 	    // Energy spectrum of decay in flight particles
    std::vector<std::vector<TH1D*> > Baseline_Parent;       // Flux by Parent vs baseline
    std::vector<std::vector<TH1D*> > DIF_Baseline_Parent;   // Flux by Parent vs baseline for DIF
    std::vector<std::vector<TH2D*> > Enu_Baseline_Parent;   // Flux by Parent Enu vs baseline


    // Other hists
    std::vector<TH1D*> PiDAR_zpos;
    std::vector<TH1D*> KDAR_zpos;
    std::vector<TH1D*> MuDAR_zpos;
    std::vector<TH1D*> parent_mom;   // momentum distribution of nu parent
    std::vector<TH1D*> parent_angle; // angle distribution of nu parent
    std::vector<TH1D*> parent_zpos;  // zpos distribution of nu parent
    std::vector<TH2D*> parent_zpos_angle;  // zpos vs  distribution of nu parent
    std::vector<TH2D*> parent_zpos_angle_energy;  // zpos vs angle vs energy of nu parent

    std::vector<TH1D*> flux_targ; // flux from target
    std::vector<TH1D*> flux_pipe; // flux from decay pipe
    std::vector<TH1D*> flux_dump; // flux from beam dump

    std::vector<TH1D*> flux_targ_theta; // flux from target with theta cut
    std::vector<TH1D*> flux_pipe_theta; // flux from decay pipe with theta cut
    std::vector<TH1D*> flux_dump_theta; // flux from beam dump with theta cut

    // 2D histograms
    std::vector<TH2D*> Enu_Th_CV_AV_TPC;
    std::vector<TH2D*> Enu_Th_UW_AV_TPC;


    // Weighted Histograms
    std::vector<std::vector<std::vector<TH1D*>>> Enu_Syst_AV_TPC;     //1D
    std::vector<std::vector<std::vector<TH2D*>>> Enu_Th_Syst_AV_TPC;  //2D

    // Loop over input arguments
    for (int i = 1; i < argc; i++) {

        std::string input = string(argv[i]);

        // For CV, we actaully have 600 universes
        if (input == "CV" ){
            n_universes = 200;

            // Add the extra input tags 
            extra_genie = true;
            continue;
        }

        std::cout << "FILE : " << argv[i] << std::endl; 
        TFile *filein = TFile::Open(argv[i]);
        
        if (filein->IsZombie()) {
            std::cout << "ERROR: File is ZOMBIE:\t" << argv[i] << std::endl;
            badfiles.push_back(string(argv[i]));
            filein->Close();
        }
        else {
            filename.push_back(string(argv[i]));
            totalPOT+=500000; // 50* 100 000 POT per dk2nu file
            filein->Close();
        }
    }

    std::cout << "\nTotal POT read in:\t" << totalPOT << std::endl;
    std::cout << "\nUsing 5e5 POT per dk2nu file, the flux will be wrong if this is not the case!\n" << std::endl;

    // Resizing of histograms
    Enu_CV_AV_TPC.resize(flav.size());
    Enu_UW_AV_TPC.resize(flav.size());
    
    Th_CV_AV_TPC.resize(flav.size());
    Th_UW_AV_TPC.resize(flav.size());
    Th_CV_AV_TPC_DIF.resize(flav.size());
    Th_UW_AV_TPC_DIF.resize(flav.size());

    Baseline_CV_AV_TPC.resize(flav.size());
    Baseline_CV_AV_TPC_DIF.resize(flav.size());
    Enu_Baseline_CV_AV_TPC_2D.resize(flav.size());
    
    
    Enu_CV_AV_TPC_5MeV_bin.resize(flav.size());
    Enu_UW_AV_TPC_5MeV_bin.resize(flav.size());

    Enu_CV_AV_TPC_5MeV_bin_DIF.resize(flav.size());
    Enu_UW_AV_TPC_5MeV_bin_DIF.resize(flav.size());
    
    Enu_Parent_AV_TPC.resize(flav.size());
    Th_Parent_AV_TPC.resize(flav.size());
    zpos_Parent_AV_TPC.resize(flav.size());
    impwght_Parent.resize(flav.size());
    Targ_mom_Parent.resize(flav.size());
    DAR_Enu_Parent.resize(flav.size());
    DAR_Th_Parent.resize(flav.size());
    DIF_Enu_Parent.resize(flav.size());
    DIF_Th_Parent.resize(flav.size());
    Baseline_Parent.resize(flav.size());
    DIF_Baseline_Parent.resize(flav.size());
    Enu_Baseline_Parent.resize(flav.size());

    // 2D
    Enu_Th_CV_AV_TPC.resize(flav.size());
    Enu_Th_UW_AV_TPC.resize(flav.size());

    // Other
    parent_mom.resize(flav.size());
    parent_angle.resize(flav.size());
    parent_zpos.resize(flav.size()); 
    PiDAR_zpos.resize(flav.size());
    KDAR_zpos.resize(flav.size()); 
    MuDAR_zpos.resize(flav.size());
    parent_zpos_angle.resize(flav.size());
    parent_zpos_angle_energy.resize(flav.size());
    flux_targ.resize(flav.size());
    flux_pipe.resize(flav.size());
    flux_dump.resize(flav.size());
    flux_targ_theta.resize(flav.size());
    flux_pipe_theta.resize(flav.size());
    flux_dump_theta.resize(flav.size());

    // Weighted
    Enu_Syst_AV_TPC.resize(flav.size());    // 1D
    Enu_Th_Syst_AV_TPC.resize(flav.size()); // 2D
    
    TFile* output = new TFile("output.root", "RECREATE");

    // Tree for POT counting
    TTree* POTTree = new TTree("POT","Total POT");
    POTTree->SetDirectory(output);
    POTTree -> Branch("POT", &totalPOT);
    POTTree -> Fill();

    

    TTree* FluxTree = new TTree("FluxTree","FluxTree");
    FluxTree->SetDirectory(output);
    FluxTree -> Branch("theta", &theta);
    FluxTree -> Branch("Enu", &Enu);
    FluxTree -> Branch("Pmom_dk", &Pmom_dk);
    // FluxTree -> Branch("Pmom_tg", &Pmom_tg);
    FluxTree -> Branch("cv_weight", &cv_weight);
    FluxTree -> Branch("dk2nu_weight", &dk2nu_weight);
    FluxTree -> Branch("tree_nu_pdg", &tree_nu_pdg);
    FluxTree -> Branch("par_pdg", &par_pdg);
    // FluxTree -> Branch("decay_zpos", &decay_zpos);
    // FluxTree -> Branch("imp_weight", &imp_weight);
    

    // Histogram Bins | 1 for each flavour + theta
	std::vector<std::vector<double>> bins(5);

    bins[0] = { // numu
            0.00, 0.025, 0.03, 0.235 ,0.24, 0.50, 0.75, 1.00, 1.25, 1.50, 1.75, 2.00, 2.25, 2.50, 2.75, 3.00, 4.00, 5.00, 6.00, 7.00, 10.00 };

    bins[1] = {  // nue
        0.00 ,0.06, 0.125, 0.25, 0.5, 0.75, 1.00, 1.25, 1.50, 1.75, 2.00, 2.25, 2.50, 2.75, 3.00, 3.25, 3.50, 4.00, 5.00 };

    bins[2] = {// numubar
            0.00, 0.025, 0.03, 0.235 ,0.24, 0.50, 0.75, 1.00, 1.25, 1.50, 1.75, 2.00, 2.25, 2.50, 2.75, 3.00, 4.00, 5.00, 6.00, 7.00, 10.00 };

    bins[3] = {  // nuebar
        0.00 ,0.06, 0.125, 0.25, 0.5, 0.75, 1.00, 1.25, 1.50, 1.75, 2.00, 2.25, 2.50, 2.75, 3.00, 3.25, 3.50, 4.00, 5.00 };
    
    // Theta
    bins[4] = {  0, 10, 20, 110, 160 }; // theta -- removed edge theta bins where no events live and split into 3 bins for stats

    
    // Resize the weight labels
    Weights.resize(labels.size());
    for (unsigned int i=0; i<labels.size(); i++) Weights[i].resize(n_universes);

    int const n_th = bins.at(flav.size()).size()-1; // theta bins
    temp2 = bins.at(4); 

    int num_bins = 4000; // Number of bins to divide the main histograms by. Was 4000

    // Flavors
    for(unsigned i=0; i<flav.size(); i++) {
        
        int const n = bins.at(i).size()-1;
        temp.clear();
        temp = bins.at(i);

        double* bin = &temp[0];
        double* bin_th = &temp2[0];

        // FLux histograms
        Enu_CV_AV_TPC[i] = new TH1D(Form("%s_CV_AV_TPC",flav[i].c_str()),"",n, bin);
        Enu_UW_AV_TPC[i] = new TH1D(Form("%s_UW_AV_TPC",flav[i].c_str()),"",n, bin);
        Th_CV_AV_TPC [i] = new TH1D(Form("Th_%s_CV_TPC", flav[i].c_str()), "", 180, 0, 180);
        Th_UW_AV_TPC [i] = new TH1D(Form("Th_%s_UW_TPC", flav[i].c_str()), "", 180, 0, 180);
        Th_CV_AV_TPC_DIF [i] = new TH1D(Form("Th_%s_CV_TPC_DIF", flav[i].c_str()), "", 180, 0, 180);
        Th_UW_AV_TPC_DIF [i] = new TH1D(Form("Th_%s_UW_TPC_DIF", flav[i].c_str()), "", 180, 0, 180);

        Baseline_CV_AV_TPC[i] = new TH1D(Form("Baseline_%s_CV_TPC", flav[i].c_str()), "", 330,   50, 710);
        Baseline_CV_AV_TPC_DIF[i] = new TH1D(Form("Baseline_%s_CV_TPC", flav[i].c_str()), "", 330,   50, 710);
        Enu_Baseline_CV_AV_TPC_2D[i] = new TH2D(Form("Enu_Baseline_%s_CV_TPC", flav[i].c_str()), "", 50, 0, 5, 100, 50, 700);
        

        // Other Histograms
        parent_mom[i]   = new TH1D(Form("%s_parent_mom",   flav[i].c_str()), "", 100, 0, 25);    // momentum distribution of nu parent
        parent_angle[i] = new TH1D(Form("%s_parent_angle", flav[i].c_str()), "", 180, 0, 180);   // angle distribution of nu parent
        parent_zpos[i]  = new TH1D(Form("%s_parent_zpos",  flav[i].c_str()), "", 400, 0, 80000); // zpos distribution of nu parent
        PiDAR_zpos[i]   = new TH1D(Form("%s_PiDAR_zpos",   flav[i].c_str()),"",  400, 0, 80000); // Pidar peak zpos
        KDAR_zpos[i]    = new TH1D(Form("%s_KDAR_zpos",    flav[i].c_str()), "", 400, 0, 80000); // Kdar peak zpos 
        MuDAR_zpos[i]   = new TH1D(Form("%s_MuDAR_zpos",   flav[i].c_str()),"",  400, 0, 80000); // Mudar peak zpos
        parent_zpos_angle[i]        = new TH2D(Form("%s_parent_zpos_angle", flav[i].c_str()), "", 200, 0, 80000, 180 , 0, 180);
        parent_zpos_angle_energy[i] = new TH2D(Form("%s_parent_zpos_angle_energy", flav[i].c_str()), "", 200, 0, 80000, 180 , 0, 180);
        flux_targ[i] = new TH1D( Form("%s_flux_targ", flav[i].c_str()),"", num_bins, 0, 20 );
        flux_pipe[i] = new TH1D( Form("%s_flux_pipe", flav[i].c_str()),"", num_bins, 0, 20 );
        flux_dump[i] = new TH1D( Form("%s_flux_dump", flav[i].c_str()),"", num_bins, 0, 20 );
        flux_targ_theta[i] = new TH1D( Form("%s_flux_targ_theta", flav[i].c_str()),"", num_bins, 0, 20 );
        flux_pipe_theta[i] = new TH1D( Form("%s_flux_pipe_theta", flav[i].c_str()),"", num_bins, 0, 20 );
        flux_dump_theta[i] = new TH1D( Form("%s_flux_dump_theta", flav[i].c_str()),"", num_bins, 0, 20 );

        // 2D
        Enu_Th_CV_AV_TPC[i] = new TH2D(Form("%s_CV_AV_TPC_2D",flav[i].c_str()),"",n, bin, n_th, bin_th);
        Enu_Th_UW_AV_TPC[i] = new TH2D(Form("%s_unweighted_AV_TPC_2D",flav[i].c_str()),"",n, bin, n_th, bin_th);

        // new binning schmeme to be the same as marcos
        Enu_CV_AV_TPC_5MeV_bin[i] = new TH1D(Form("%s_CV_AV_TPC_5MeV_bin",flav[i].c_str()),"",num_bins, 0, 20);
        Enu_UW_AV_TPC_5MeV_bin[i] = new TH1D(Form("%s_UW_AV_TPC_5MeV_bin",flav[i].c_str()),"",num_bins, 0, 20);

        Enu_CV_AV_TPC_5MeV_bin_DIF[i] = new TH1D(Form("%s_CV_AV_TPC_5MeV_bin_DIF",flav[i].c_str()),"",num_bins, 0, 20);
        Enu_UW_AV_TPC_5MeV_bin_DIF[i] = new TH1D(Form("%s_UW_AV_TPC_5MeV_bin_DIF",flav[i].c_str()),"",num_bins, 0, 20);

        Enu_Parent_AV_TPC[i].resize(parent.size());
        Th_Parent_AV_TPC[i].resize(parent.size());
        zpos_Parent_AV_TPC[i].resize(parent.size());
        impwght_Parent[i].resize(parent.size());
        Targ_mom_Parent[i].resize(parent.size());
        DAR_Enu_Parent[i].resize(parent.size());
        DAR_Th_Parent[i].resize(parent.size());
        DIF_Enu_Parent[i].resize(parent.size());
        DIF_Th_Parent[i].resize(parent.size());
        Baseline_Parent[i].resize(parent.size());
        DIF_Baseline_Parent[i].resize(parent.size());
        Enu_Baseline_Parent[i].resize(parent.size());
        
        // Parent
        for(unsigned k = 0; k < parent.size(); k++){
            Enu_Parent_AV_TPC[i][k]  = new TH1D(Form("Enu_%s_%s_AV_TPC",     flav[i].c_str(), parent[k].c_str()),"", num_bins, 0, 20);
            Th_Parent_AV_TPC[i][k]   = new TH1D(Form("Th_%s_%s_AV_TPC",      flav[i].c_str(), parent[k].c_str()),"", 180,   0, 180);
            zpos_Parent_AV_TPC[i][k] = new TH1D(Form("zpos_%s_%s_AV_TPC",    flav[i].c_str(), parent[k].c_str()),"", 400,  0, 80000);
            impwght_Parent[i][k]     = new TH1D(Form("impwght_Parent_%s_%s", flav[i].c_str(), parent[k].c_str()),"", 1000, 0, 1000);
            Targ_mom_Parent[i][k]    = new TH1D(Form("Targ_mom_Parent_%s_%s",flav[i].c_str(), parent[k].c_str()),"", num_bins, 0, 20);
            DAR_Enu_Parent[i][k]     = new TH1D(Form("DAR_Enu_%s_%s_AV_TPC", flav[i].c_str(), parent[k].c_str()),"", num_bins, 0, 20);
            DAR_Th_Parent[i][k]     = new TH1D(Form("DAR_Th_%s_%s_AV_TPC", flav[i].c_str(), parent[k].c_str()),"", 180,   0, 180);
            DIF_Enu_Parent[i][k]     = new TH1D(Form("DIF_Enu_%s_%s_AV_TPC", flav[i].c_str(), parent[k].c_str()),"", num_bins, 0, 20);
            DIF_Th_Parent[i][k]     = new TH1D(Form("DIF_Th_%s_%s_AV_TPC", flav[i].c_str(), parent[k].c_str()),"", 180,   0, 180);
            Baseline_Parent[i][k]   = new TH1D(Form("Baseline_Parent_%s_%s_AV_TPC", flav[i].c_str(), parent[k].c_str()),"", 330,   50, 710);
            DIF_Baseline_Parent[i][k] = new TH1D(Form("DIF_Baseline_Parent_%s_%s_AV_TPC", flav[i].c_str(), parent[k].c_str()),"", 330,   50, 710);
            Enu_Baseline_Parent[i][k] = new TH2D(Form("Enu_Baseline_Parent_%s_%s_AV_TPC", flav[i].c_str(), parent[k].c_str()),"", 50, 0, 5, 100, 50, 700);
        }
        
        // Weighted Stuff
        Enu_Syst_AV_TPC[i].resize(labels.size());
        Enu_Th_Syst_AV_TPC[i].resize(labels.size());
        
        // Labels
        for(unsigned j=0; j<labels.size(); j++) {
            Enu_Syst_AV_TPC[i][j].resize(n_universes);
            Enu_Th_Syst_AV_TPC[i][j].resize(n_universes);

            // Universes
            for(int k=0; k<n_universes; k++){
                // Shift the universes for the different sets
                int index_uni = k;
                
                if (set == 2 ){
                    index_uni+=200;
                }
                
                if (set == 3 ){
                    index_uni+=400;
                }
                
                Enu_Syst_AV_TPC[i][j][k]    =  new TH1D(Form("%s_%s_Uni_%d_AV_TPC",flav[i].c_str(), labels[j].c_str(), index_uni),"",n, bin);
                Enu_Th_Syst_AV_TPC[i][j][k] =  new TH2D(Form("%s_%s_Uni_%d_AV_TPC_2D",flav[i].c_str(), labels[j].c_str(), index_uni),"",n, bin, n_th, bin_th);
            }
        }
    }

    // ++++++++++++++++++++++++++++++++
    // Event loop
    // ++++++++++++++++++++++++++++++++
    std::cout << "Starting Eventloop" << std::endl;
    bool disperrors{false}; // chose whether or not to display the unphysical nuetrino errors

    int n = 0;

    // Loop over events
    for (gallery::Event ev(filename); !ev.atEnd(); ev.next()) {
        n++;

        // Alert the user
        if (n % 10000 == 0) std::cout << "On entry " << n/10000.0 <<"0k" << std::endl;

        // if (n == 100000) break; 
        // if (n == 30) break; 

        auto const& mctruths = *ev.getValidHandle<vector<simb::MCTruth>>(mctruths_tag);   
        auto const& mcfluxs  = *ev.getValidHandle<vector<simb::MCFlux>>(mctruths_tag);   

        gallery::Handle<std::vector<evwgh::MCEventWeight>> ew_handle, ew_handle_extra1;

        
        std::vector<art::Ptr<evwgh::MCEventWeight>> evtwghts, evtwghts_extra1;

        ev.getByLabel(eventweight_tag_00, ew_handle);
        fill_ptr_vector(evtwghts, ew_handle);
        if(!ew_handle.isValid())std::cout << "Error the handle for eventweight_tag_00 was not valid"<< std::endl;

        if (extra_genie){

            ev.getByLabel(eventweight_tag_01, ew_handle_extra1);
            fill_ptr_vector(evtwghts_extra1, ew_handle_extra1);
            if(!ew_handle_extra1.isValid())std::cout << "Error the handle for eventweight_tag_01 was not valid"<< std::endl;
            
        }

        // Loop over MCTruths
        for (size_t i=0; i<mctruths.size(); i++) {
            auto const& mctruth = mctruths.at(i);
            auto const& mcflux  = mcfluxs.at(i);
            std::map<std::string, std::vector<double>> evtwgt_map, evtwght_map_extra1, evtwght_map_extra2, evtwght_map_extra3,evtwght_map_extra4, evtwght_map_extra5;
            
            evtwgt_map = evtwghts.at(i)->fWeight;

            if (extra_genie){
                evtwght_map_extra1 = evtwghts_extra1.at(i)->fWeight;
                
            }

            int pdg;
            if     (mctruth.GetNeutrino().Nu().PdgCode() == 14) pdg = 0; // numu
            else if(mctruth.GetNeutrino().Nu().PdgCode() == 12) pdg = 1; // nue
            else if(mctruth.GetNeutrino().Nu().PdgCode() ==-14) pdg = 2; // numubar
            else if(mctruth.GetNeutrino().Nu().PdgCode() ==-12) pdg = 3; // nuebar
            else {
                if (disperrors){
                    std::cout << "Unknown neutrino PDG: "
                        << mctruth.GetNeutrino().Nu().PdgCode()
                        << std::endl;
                }
                continue;
            }

            tree_nu_pdg = mctruth.GetNeutrino().Nu().PdgCode();

            cv_weight = 1.0;    // PPFX CV
            dk2nu_weight = 1.0; // dk2nu CV
            
            // Now get the momentums to calculate theta
            TVector3 mom_det = {mctruth.GetNeutrino().Nu().Px(),mctruth.GetNeutrino().Nu().Py(),mctruth.GetNeutrino().Nu().Pz()};
            TVector3 mom_beam = FromDetToBeam(mom_det, true); // Rotate vector to beam coordinates
            mom_beam = mom_beam.Unit();
            TVector3 beam_dir = {0 , 0 , 1};
            theta = mom_beam.Angle(beam_dir) * 180 / 3.1415926;

            Enu = mctruth.GetNeutrino().Nu().E();
            Pmom_dk = std::sqrt( mcflux.fpdpz*mcflux.fpdpz + mcflux.fpdpy*mcflux.fpdpy + mcflux.fpdpx*mcflux.fpdpx ); // Parent momentum at decay
            Pmom_tg = std::sqrt(mcflux.ftpx*mcflux.ftpx + mcflux.ftpy*mcflux.ftpy + mcflux.ftpz*mcflux.ftpz);         // Parent moment

            if (evtwgt_map.find("ppfx_cv_UBPPFXCV") != evtwgt_map.end()) {

                // if(last.second.at(0) > 30 || last.second.at(0) < 0){ // still fill even if bad weight, changed from >90 to >30
                if(evtwgt_map.find("ppfx_cv_UBPPFXCV")->second.at(0) < 0 || evtwgt_map.find("ppfx_cv_UBPPFXCV")->second.at(0) > 50 ){ 
                    std::cout << "Bad CV weight, setting to 1: " << evtwgt_map.find("ppfx_cv_UBPPFXCV")->second.at(0) << std::endl;
                    cv_weight = 1;
                    // cv_weight = last.second.at(0);
                }
                else {
                    cv_weight     = evtwgt_map.find("ppfx_cv_UBPPFXCV")->second.at(0);
                    // std::cout << "CV weight:\t" << cv_weight << std::endl;
                }  

            }

            
            // Weight of neutrino parent (importance weight) * Neutrino weight for a decay forced at center of near detector 
            cv_weight        *= mcflux.fnimpwt * mcflux.fnwtnear; // for ppfx cases
            dk2nu_weight     *= mcflux.fnimpwt * mcflux.fnwtnear; // for UW cases 
            
            // Error handling, sets to zero if bad
            check_weight(cv_weight);
            check_weight(dk2nu_weight);

            // Fill Weight vector with 1's to create size=labels
            for (unsigned l=0; l<labels.size(); l++) {
                std::fill(Weights[l].begin(), Weights[l].end(), 1);
            }

            // Get the PPFX weights
            GetPPFXWeights(Weights, 0, evtwgt_map, labels);

            // Get the extra weights
            if (extra_genie){
                GetPPFXWeights(Weights, 100, evtwght_map_extra1, labels);
                
            }


            // ++++++++++++++++++++++++++++++++
            // Now got weights, fill histograms
            // ++++++++++++++++++++++++++++++++

            par_pdg = mcflux.fptype;
            decay_zpos = mcflux.fvz;
            imp_weight  = mcflux.fnimpwt;

            TVector3 Trans_Targ2Det_beam = {5502, 7259, 67270};
            TVector3 Decay_pos = {mcflux.fvx, mcflux.fvy, mcflux.fvz};

            // std::cout << mcflux.fvx << " " << mcflux.fvy << " " << mcflux.fvz << std::endl;
            TVector3 baseline_vec = Trans_Targ2Det_beam - Decay_pos;
            double baseline = baseline_vec.Mag()/100.0;
            // std::cout << "Baseline: "<< baseline << std::endl; 
            

            // TPC AV
            Enu_CV_AV_TPC[pdg]              ->Fill(Enu, cv_weight);
            Enu_UW_AV_TPC[pdg]              ->Fill(Enu, dk2nu_weight);
            Enu_CV_AV_TPC_5MeV_bin[pdg]     ->Fill(Enu, cv_weight);
            Enu_UW_AV_TPC_5MeV_bin[pdg]     ->Fill(Enu, dk2nu_weight);
            Th_CV_AV_TPC[pdg]               ->Fill(theta, cv_weight);
            Th_UW_AV_TPC[pdg]               ->Fill(theta, dk2nu_weight);
            Baseline_CV_AV_TPC[pdg]         ->Fill(baseline, cv_weight);
            Enu_Baseline_CV_AV_TPC_2D[pdg]  ->Fill(Enu, baseline, cv_weight);

            if (Pmom_dk != 0 ) {
                Th_CV_AV_TPC_DIF[pdg]               ->Fill(theta, cv_weight);
                Th_UW_AV_TPC_DIF[pdg]               ->Fill(theta, dk2nu_weight);
                Enu_CV_AV_TPC_5MeV_bin_DIF[pdg]     ->Fill(Enu, cv_weight);
                Enu_UW_AV_TPC_5MeV_bin_DIF[pdg]     ->Fill(Enu, dk2nu_weight);
                Baseline_CV_AV_TPC_DIF[pdg]         ->Fill(baseline, cv_weight);
            }

            // INDEXING: 0: PI_Plus 1: PI_Minus 2: Mu Plus 3: Mu_Minus 4: Kaon_Plus 5: Kaon_Minus 6: K0L 
            if (mcflux.fptype == 211){ // pi plus
                Enu_Parent_AV_TPC[pdg][0]  ->Fill(Enu, cv_weight);
                Th_Parent_AV_TPC[pdg][0]   ->Fill(theta, cv_weight);
                zpos_Parent_AV_TPC[pdg][0] ->Fill(mcflux.fvz, cv_weight);
                impwght_Parent[pdg][0]     ->Fill(mcflux.fnimpwt);
                Targ_mom_Parent[pdg][0]    ->Fill(Pmom_tg, cv_weight);
                Baseline_Parent[pdg][0]    ->Fill(baseline, cv_weight);
                Enu_Baseline_Parent[pdg][0]    ->Fill(Enu, baseline, cv_weight);

                // Fill DAR Energy spectrum
                if (Pmom_dk == 0 ) {
                    DAR_Enu_Parent[pdg][0]->Fill(Enu, cv_weight);
                    DAR_Th_Parent[pdg][0]->Fill(theta, cv_weight);
                }
                else {
                    DIF_Enu_Parent[pdg][0]->Fill(Enu, cv_weight);
                    DIF_Th_Parent[pdg][0]->Fill(theta, cv_weight);
                    DIF_Baseline_Parent[pdg][0]->Fill(baseline, cv_weight);
                }
                
            }
            else if (mcflux.fptype == -211){ // pi minus
                Enu_Parent_AV_TPC[pdg][1]  ->Fill(Enu, cv_weight);
                Th_Parent_AV_TPC[pdg][1]   ->Fill(theta, cv_weight);
                zpos_Parent_AV_TPC[pdg][1] ->Fill(mcflux.fvz, cv_weight);
                impwght_Parent[pdg][1]     ->Fill(mcflux.fnimpwt);
                Targ_mom_Parent[pdg][1]    ->Fill(Pmom_tg, cv_weight);
                Baseline_Parent[pdg][1]    ->Fill(baseline, cv_weight);
                Enu_Baseline_Parent[pdg][1]    ->Fill(Enu, baseline, cv_weight);

                // Fill DAR Energy spectrum
                if (Pmom_dk == 0 ){
                    DAR_Enu_Parent[pdg][1]->Fill(Enu, cv_weight);
                    DAR_Th_Parent[pdg][1]->Fill(theta, cv_weight);
                }
                else {
                    DIF_Enu_Parent[pdg][1]->Fill(Enu, cv_weight);
                    DIF_Th_Parent[pdg][1]->Fill(theta, cv_weight);
                    DIF_Baseline_Parent[pdg][1]->Fill(baseline, cv_weight);
                }
                
            }
            else if (mcflux.fptype == -13){ // mu plus
                Enu_Parent_AV_TPC[pdg][2]  ->Fill(Enu, cv_weight);
                Th_Parent_AV_TPC[pdg][2]   ->Fill(theta, cv_weight);
                zpos_Parent_AV_TPC[pdg][2] ->Fill(mcflux.fvz, cv_weight);
                impwght_Parent[pdg][2]     ->Fill(mcflux.fnimpwt);
                Targ_mom_Parent[pdg][2]    ->Fill(Pmom_tg, cv_weight);
                Baseline_Parent[pdg][2]    ->Fill(baseline, cv_weight);
                Enu_Baseline_Parent[pdg][2]    ->Fill(Enu, baseline, cv_weight);
    
                // Fill DAR Energy spectrum
                if (Pmom_dk == 0 ){
                    DAR_Enu_Parent[pdg][2]->Fill(Enu, cv_weight);
                    DAR_Th_Parent[pdg][2]->Fill(theta, cv_weight);
                }
                else {
                    DIF_Enu_Parent[pdg][2]->Fill(Enu, cv_weight);
                    DIF_Th_Parent[pdg][2]->Fill(theta, cv_weight);
                    DIF_Baseline_Parent[pdg][2]->Fill(baseline, cv_weight);
                }

            } 
            else if (mcflux.fptype == 13){ // mu minus
                Enu_Parent_AV_TPC[pdg][3]  ->Fill(Enu, cv_weight);
                Th_Parent_AV_TPC[pdg][3]   ->Fill(theta, cv_weight);
                zpos_Parent_AV_TPC[pdg][3] ->Fill(mcflux.fvz, cv_weight);
                impwght_Parent[pdg][3]     ->Fill(mcflux.fnimpwt);
                Targ_mom_Parent[pdg][3]    ->Fill(Pmom_tg, cv_weight);
                Baseline_Parent[pdg][3]    ->Fill(baseline, cv_weight);
                Enu_Baseline_Parent[pdg][3]    ->Fill(Enu, baseline, cv_weight);

                // Fill DAR Energy spectrum
                if (Pmom_dk == 0 ){
                    DAR_Enu_Parent[pdg][3]->Fill(Enu, cv_weight);
                    DAR_Th_Parent[pdg][3]->Fill(theta, cv_weight);
                }
                else {
                    DIF_Enu_Parent[pdg][3]->Fill(Enu, cv_weight);
                    DIF_Th_Parent[pdg][3]->Fill(theta, cv_weight);
                    DIF_Baseline_Parent[pdg][3]->Fill(baseline, cv_weight);
                }

            } 
            else if (mcflux.fptype == 321){ // K+
                Enu_Parent_AV_TPC[pdg][4]  ->Fill(Enu, cv_weight);
                Th_Parent_AV_TPC[pdg][4]   ->Fill(theta, cv_weight);
                zpos_Parent_AV_TPC[pdg][4] ->Fill(mcflux.fvz, cv_weight);
                impwght_Parent[pdg][4]     ->Fill(mcflux.fnimpwt);
                Targ_mom_Parent[pdg][4]    ->Fill(Pmom_tg, cv_weight);
                Baseline_Parent[pdg][4]    ->Fill(baseline, cv_weight);
                Enu_Baseline_Parent[pdg][4]    ->Fill(Enu, baseline, cv_weight);

                // Fill DAR Energy spectrum
                if (Pmom_dk == 0 ){
                    DAR_Enu_Parent[pdg][4]->Fill(Enu, cv_weight);
                    DAR_Th_Parent[pdg][4]->Fill(theta, cv_weight);
                }
                else {
                    DIF_Enu_Parent[pdg][4]->Fill(Enu, cv_weight);
                    DIF_Th_Parent[pdg][4]->Fill(theta, cv_weight);
                    DIF_Baseline_Parent[pdg][4]->Fill(baseline, cv_weight);
                }
                
            }
            else if ( mcflux.fptype == -321){ // K-
                Enu_Parent_AV_TPC[pdg][5]  ->Fill(Enu, cv_weight);
                Th_Parent_AV_TPC[pdg][5]   ->Fill(theta, cv_weight);
                zpos_Parent_AV_TPC[pdg][5] ->Fill(mcflux.fvz, cv_weight);
                impwght_Parent[pdg][5]     ->Fill(mcflux.fnimpwt);
                Targ_mom_Parent[pdg][5]    ->Fill(Pmom_tg, cv_weight);
                Baseline_Parent[pdg][5]    ->Fill(baseline, cv_weight);
                Enu_Baseline_Parent[pdg][5]    ->Fill(Enu, baseline, cv_weight);

                // Fill DAR Energy spectrum
                if (Pmom_dk == 0 ){
                    DAR_Enu_Parent[pdg][5]->Fill(Enu, cv_weight);
                    DAR_Th_Parent[pdg][5]->Fill(theta, cv_weight);
                }
                else {
                    DIF_Enu_Parent[pdg][5]->Fill(Enu, cv_weight);
                    DIF_Th_Parent[pdg][5]->Fill(theta, cv_weight);
                    DIF_Baseline_Parent[pdg][5]->Fill(baseline, cv_weight);
                }
                
            }
            else if (mcflux.fptype == 130 ){ // K0L
                Enu_Parent_AV_TPC[pdg][6]  ->Fill(Enu, cv_weight);
                Th_Parent_AV_TPC[pdg][6]   ->Fill(theta, cv_weight);
                zpos_Parent_AV_TPC[pdg][6] ->Fill(mcflux.fvz, cv_weight);
                impwght_Parent[pdg][6]     ->Fill(mcflux.fnimpwt);
                Targ_mom_Parent[pdg][6]    ->Fill(Pmom_tg, cv_weight);
                Baseline_Parent[pdg][6]    ->Fill(baseline, cv_weight);
                Enu_Baseline_Parent[pdg][6]    ->Fill(Enu, baseline, cv_weight);

                // Fill DAR Energy spectrum
                if (Pmom_dk == 0 ){
                    DAR_Enu_Parent[pdg][6]->Fill(Enu, cv_weight);
                    DAR_Th_Parent[pdg][6]->Fill(theta, cv_weight);
                }
                else {
                    DIF_Enu_Parent[pdg][6]->Fill(Enu, cv_weight);
                    DIF_Th_Parent[pdg][6]->Fill(theta, cv_weight);
                    DIF_Baseline_Parent[pdg][6]->Fill(baseline, cv_weight);
                }

            }
            
            if (Pmom_dk == 0 && (mcflux.fptype == 211 || mcflux.fptype == -211) ){ // Pidar peak
                PiDAR_zpos[pdg]->Fill(mcflux.fvz, cv_weight);
            }
            if (Pmom_dk == 0 && (mcflux.fptype == 321 || mcflux.fptype == -321)){  // Kdar peak
                KDAR_zpos[pdg]->Fill(mcflux.fvz, cv_weight);
            }
            if (Pmom_dk == 0 && (mcflux.fptype == 13 || mcflux.fptype == -13)){    // Mudar peak
                MuDAR_zpos[pdg]->Fill(mcflux.fvz, cv_weight);
            }

            // Other plots
            parent_mom[pdg]  ->Fill(Pmom_tg,    cv_weight);
            parent_angle[pdg]->Fill(theta,      cv_weight);
            parent_zpos[pdg] ->Fill(mcflux.fvz, cv_weight);
            parent_zpos_angle[pdg]        ->Fill(mcflux.fvz, theta, cv_weight);
            parent_zpos_angle_energy[pdg]->Fill(mcflux.fvz, theta, Enu);
        
            if ( mcflux.fvz < 10000) flux_targ[pdg] ->Fill(Enu, cv_weight);
            if ( mcflux.fvz > 10000 &&  mcflux.fvz < 72000) flux_pipe[pdg] ->Fill(Enu, cv_weight);
            if ( mcflux.fvz > 72000) flux_dump[pdg] ->Fill(Enu, cv_weight);

            if ( theta < 20) flux_targ_theta[pdg] ->Fill(Enu, cv_weight);
            if ( theta > 20 &&  theta < 110) flux_pipe_theta[pdg] ->Fill(Enu, cv_weight);
            if ( theta > 110) flux_dump_theta[pdg] ->Fill(Enu, cv_weight);

            // 2D Stuff
            Enu_Th_CV_AV_TPC[pdg]->Fill(Enu, theta, cv_weight);
            Enu_Th_UW_AV_TPC[pdg]->Fill(Enu, theta, dk2nu_weight);

           

            // Now fill multisims
            for (unsigned l=0; l<labels.size(); l++) {

                // Universes
                for (unsigned i=0; i<Weights[l].size(); i++) {
                    
                    Enu_Syst_AV_TPC[pdg][l][i]->Fill(Enu, Weights[l][i]*cv_weight);
                    Enu_Th_Syst_AV_TPC[pdg][l][i]->Fill(Enu, theta, Weights[l][i]*cv_weight);
                }
            }

            FluxTree -> Fill();

        } // End loop over mctruth

    } // End loop over events


    // ++++++++++++++++++++++++++++++++
    // Plotting 
    // ++++++++++++++++++++++++++++++++
    output->cd();
    TDirectory* savdir = gDirectory;

    std::cout << "flavour.size:\t" <<flav.size()<<std::endl;

    // Top Flav dir 
    std::vector<std::vector<TDirectory*>> subdir(flav.size());

    //Create label dirs
    for (unsigned i=0; i<flav.size(); i++){
        subdir[i].resize(parent.size()+5);
    
    }

    // Flavours
    for (unsigned int f=0; f<flav.size(); f++) {
    
        std::cout << "\n" <<flav[f] << std::endl;

        subdir[f][0] = savdir->mkdir(Form("%s",flav[f].c_str()));
        subdir[f][0]->cd();

        // Parent
        // INDEXING: 1: PI_Plus 2: PI_Minus 3: Mu Plus 4: Mu_Minus 5: Kaon_Plus 6: Kaon_Minus 7: K0L 
        for(unsigned int k = 1; k < parent.size()+1; k++){
            std::cout << parent[k-1] << std::endl;
            subdir[f][k] = subdir[f][0]->mkdir(Form("%s",parent[k-1].c_str()));
            subdir[f][k]->cd();

            Enu_Parent_AV_TPC[f][k-1]->Write();
            Th_Parent_AV_TPC[f][k-1]->Write();
            zpos_Parent_AV_TPC[f][k-1]->Write();
            impwght_Parent[f][k-1]->Write();
            Targ_mom_Parent[f][k-1]->Write();
            DAR_Enu_Parent[f][k-1]->Write();
            DAR_Th_Parent[f][k-1]->Write();
            DIF_Enu_Parent[f][k-1]->Write();
            DIF_Th_Parent[f][k-1]->Write();
            Baseline_Parent[f][k-1]->Write();
            DIF_Baseline_Parent[f][k-1]->Write();
            Enu_Baseline_Parent[f][k-1]->Write();
    
        }

        // Make other plots folder for miscalanious variables
        std::cout << "OtherPlots" << std::endl;
        subdir.at(f).at(parent.size()+1) = subdir[f][0]->mkdir("OtherPlots");
        subdir.at(f).at(parent.size()+1)->cd();
        
        PiDAR_zpos[f]->Write();
        KDAR_zpos[f]->Write();
        MuDAR_zpos[f]->Write();
        parent_mom[f]->Write();
        parent_angle[f]->Write();
        parent_zpos[f]->Write();
        parent_zpos_angle[f]->Write();
        parent_zpos_angle_energy[f]->Write();
        flux_targ[f]->Write();
        flux_pipe[f]->Write();
        flux_dump[f]->Write();
        flux_targ_theta[f]->Write();
        flux_pipe_theta[f]->Write();
        flux_dump_theta[f]->Write();

        std::cout << "Detsmear" << std::endl;
        subdir.at(f).at(parent.size()+3) = subdir[f][0]->mkdir("Detsmear");
        subdir.at(f).at(parent.size()+3)->cd();
        Enu_CV_AV_TPC[f]->Write();
        Enu_UW_AV_TPC[f]->Write();
        Th_CV_AV_TPC[f]->Write();
        Th_UW_AV_TPC[f]->Write();
        Th_CV_AV_TPC_DIF[f]->Write();
        Th_UW_AV_TPC_DIF[f]->Write();
        Enu_CV_AV_TPC_5MeV_bin[f]->Write();
        Enu_UW_AV_TPC_5MeV_bin[f]->Write();
        Enu_CV_AV_TPC_5MeV_bin_DIF[f]->Write();
        Enu_UW_AV_TPC_5MeV_bin_DIF[f]->Write();
        Enu_Th_CV_AV_TPC[f]->Write();
        Enu_Th_UW_AV_TPC[f]->Write();
        Baseline_CV_AV_TPC[f]->Write();
        Baseline_CV_AV_TPC_DIF[f]->Write();
        Enu_Baseline_CV_AV_TPC_2D[f]->Write();

        std::cout << "Multisims" << std::endl;
        subdir.at(f).at(parent.size()+4) = subdir[f][0]->mkdir("Multisims");
        subdir.at(f).at(parent.size()+4)->cd();
        
        for (unsigned p=0; p < labels.size(); p++){
            
            for(int i = 0; i < n_universes; i++){
                Enu_Syst_AV_TPC.at(f).at(p).at(i)->Write();
                Enu_Th_Syst_AV_TPC.at(f).at(p).at(i)->Write();
            }
        }
        savdir->cd();
    }

    POTTree->Write();
    // FluxTree->Write();

    output->Close();

    std::cout << "BADFiles:" << std::endl;
    for (unsigned int i=0;i < badfiles.size(); i++ ){
        std::cout << badfiles.at(i) << std::endl;
    }

    auto stop = high_resolution_clock::now();  // end time of script
    auto duration_sec = duration_cast<seconds>(stop - start); // time taken to run script
    auto duration_min = duration_cast<minutes>(stop - start); // time taken to run script
    std::cout << "Time taken by function: " << duration_sec.count() << " seconds" << std::endl; 
    std::cout << "Time taken by function: " << duration_min.count() << " minutes" << std::endl; 

    return 0;
}
