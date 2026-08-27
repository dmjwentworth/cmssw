#include <map>
#include <string>
#include <cmath>
#include <vector>
#include "TH1.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Framework/interface/global/EDAnalyzer.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "DataFormats/Math/interface/LorentzVector.h"
#include "SimDataFormats/TruthInfo/interface/Graph.h"
#include "PhysicsTools/TruthInfo/interface/Branch.h"
#include "SimDataFormats/TruthInfo/interface/LogicalGraphHitIndex.h"
#include "DataFormats/TauReco/interface/PFTau.h"

class DYToTauTauAnalyser : public edm::global::EDAnalyzer<> {
public:
  explicit DYToTauTauAnalyser(edm::ParameterSet const& cfg)
    : graphToken_(consumes<truth::Graph>(cfg.getParameter<edm::InputTag>("src"))),
      hitIndexToken_(consumes<truth::LogicalGraphHitIndex>(cfg.getParameter<edm::InputTag>("hitIndex"))),
      recoTauToken_(consumes<reco::PFTauCollection>(cfg.getParameter<edm::InputTag>("recoTauCollection"))) {}

  std::vector<truth::Particle> expandIntermediates(const std::vector<truth::Particle>& particles) const;
  std::vector<int32_t> getExpandedPdgIds(const std::vector<truth::Particle>& particles) const;
  int getGenTauDecayMode(const truth::Branch& tauBranch) const;
  int getPFTauDecayMode(const reco::PFTau& tau) const;

private:
  void analyze(edm::StreamID, edm::Event const& event, edm::EventSetup const&) const override;
  void beginJob() override;
  void endJob() override;

  const edm::EDGetTokenT<truth::Graph> graphToken_;
  const edm::EDGetTokenT<truth::LogicalGraphHitIndex> hitIndexToken_;
  const edm::EDGetTokenT<reco::PFTauCollection> recoTauToken_;

  std::map<std::string, TH1F*> histograms_;
  static const std::array<uint32_t, 10> doNotExpand_;
};

const std::array<uint32_t, 10> DYToTauTauAnalyser::doNotExpand_ = {
  11,  // electron
  12,  // electron neutrino
  13,  // muon
  14,  // muon neutrino
  16,  // tau neutrino
  22,  // photon
  111, // pi0
  130, // K0L
  211, // pi
  321, // K
};

std::vector<truth::Particle> DYToTauTauAnalyser::expandIntermediates(const std::vector<truth::Particle>& particles) const {
  std::vector<truth::Particle> expanded;

  for (const truth::Particle& p : particles) {
    uint32_t pdgId = std::abs(p.pdgId());
    if (std::find(doNotExpand_.begin(), doNotExpand_.end(), pdgId) != doNotExpand_.end()) {
      expanded.push_back(p);
    }
    else {
      const std::vector<truth::Particle> kids = p.children();
      if (kids.empty()) {
        std::cout << "Warning: particle with pdgId " << p.pdgId()
                  << " has no children, but is not in doNotExpand_ list." << std::endl;
        expanded.push_back(p);
      }
      else {
        for (const truth::Particle& kid : kids) {
          if (kid.hasGen()) {
            expanded.push_back(kid);
          }
        }
      }
    }
  }

  return expanded;
}

std::vector<int32_t> DYToTauTauAnalyser::getExpandedPdgIds(const std::vector<truth::Particle>& particles) const {
  std::vector<truth::Particle> temp = particles;

  while (true) {
    std::size_t n_checked = 0;
    for (const truth::Particle& p : temp) {
      uint32_t pdgId = std::abs(p.pdgId());
      const std::vector<truth::Particle> kids = p.children();
      if ((std::find(doNotExpand_.begin(), doNotExpand_.end(), pdgId) != doNotExpand_.end())
           || kids.empty()) {
        n_checked++;
      }
      else {
        n_checked = 0;
        temp = expandIntermediates(temp);
        break;
      }
    }
    if (n_checked == temp.size()) break;
  }

  std::vector<int32_t> pdgIds;
  for (const truth::Particle& p : temp) {
    pdgIds.push_back(p.pdgId());
  }
  return pdgIds;
}

int DYToTauTauAnalyser::getGenTauDecayMode(const truth::Branch& tauBranch) const {
  int decayMode{ 7 }; // default to other
  bool printDebug{ false };
  const truth::Particle tau = tauBranch.root();
  const std::vector<truth::Particle> tauDaughters = tau.children();
  const std::vector<int32_t> tauDescendants = getExpandedPdgIds(tauDaughters);

  int nEle{0}, nMu{0}, nPi0{0}, nK0{0}, nPr{0};
  for (const int32_t signedPdgId : tauDescendants) {
    uint32_t pdgId = std::abs(signedPdgId);
    if (pdgId == 22 || pdgId == 12 || pdgId == 14 || pdgId == 16) continue; // skip photons and neutrinos
    else if (pdgId == 11) nEle++;
    else if (pdgId == 13) nMu++;
    else if (pdgId == 111) nPi0++;
    else if (pdgId == 130 || pdgId == 310) nK0++;
    else if (pdgId == 211 || pdgId == 321) nPr++;
    else printDebug = true;
  }

  const std::array<int, 5> decayModeArray = { nEle, nMu, nPi0, nK0, nPr };
  if (decayModeArray == std::array<int, 5>{1, 0, 0, 0, 0}) decayMode = 0; // tau -> e
  else if (decayModeArray == std::array<int, 5>{0, 1, 0, 0, 0}) decayMode = 1; // tau -> mu
  else if (decayModeArray == std::array<int, 5>{0, 0, 0, 0, 1}) decayMode = 2; // tau -> h
  else if (decayModeArray == std::array<int, 5>{0, 0, 1, 0, 1}) decayMode = 3; // tau -> h pi0
  else if (decayModeArray == std::array<int, 5>{0, 0, 2, 0, 1}) decayMode = 4; // tau -> h pi0 pi0
  else if (decayModeArray == std::array<int, 5>{0, 0, 0, 0, 3}) decayMode = 5; // tau -> h h h
  else if (decayModeArray == std::array<int, 5>{0, 0, 1, 0, 3}) decayMode = 6; // tau -> h h h pi0
  else printDebug = true; // other

  if (printDebug) {
    std::cout << "Direct tau children: ";
    for (const truth::Particle& daughter : tauDaughters) {
      std::cout << daughter.pdgId() << ", ";
    }
    std::cout << std::endl;
    std::cout << "Expanded with my method: ";
    for (const int32_t signedPdgId : tauDescendants) {
      std::cout << signedPdgId << ", ";
    }
    std::cout << std::endl;
  }

  return decayMode;
}

int DYToTauTauAnalyser::getPFTauDecayMode(const reco::PFTau& tau) const {
  int decayMode{ 7 }; // default to other
  reco::PFTau::hadronicDecayMode decayMode_ = tau.decayMode();
  if (decayMode_ == reco::PFTau::kOneProng0PiZero) decayMode = 2; // tau -> h
  else if (decayMode_ == reco::PFTau::kOneProng1PiZero) decayMode = 3; // tau -> h pi0
  else if (decayMode_ == reco::PFTau::kOneProng2PiZero) decayMode = 4; // tau -> h pi0 pi0
  else if (decayMode_ == reco::PFTau::kThreeProng0PiZero) decayMode = 5; // tau -> h h h
  else if (decayMode_ == reco::PFTau::kThreeProng1PiZero) decayMode = 6; // tau -> h h h pi0
  else {
    std::cout << "HPS PFTau decay mode " << decayMode_ << " not recognized, setting to 'other'." << std::endl;
  }
  return decayMode;
}

void DYToTauTauAnalyser::analyze(edm::StreamID, edm::Event const& event, edm::EventSetup const&) const {
  std::cout << "\nProcessing event " << event.id() << std::endl;

  auto const& graph = event.get(graphToken_);          // truth::Graph
  auto const& hits  = event.get(hitIndexToken_);       // truth::LogicalGraphHitIndex
  auto const& recoTaus = event.get(recoTauToken_);     // std::vector<reco::PFTau>

  for (truth::Particle p : graph.particleViews()) { // loop over all particles
    if (!p.valid()) {
      continue; // skip invalid particles
    }
    
    if (p.pdgId() == 23 && p.hasGen()) { // Z boson at generator level
      const truth::Branch bosonBranch(&graph, p.id());
      
      if (bosonBranch.isSignal()) {
        // histograms_.at("GenZMass")->Fill(p.momentum().mass());
        // histograms_.at("mTauTauVis")->Fill(bosonBranch.visibleP4().mass());
        // math::XYZTLorentzVectorD pMiss = bosonBranch.p4() - bosonBranch.visibleP4();
        // histograms_.at("pTmiss")->Fill(pMiss.pt());

        std::vector<truth::Particle> bosonDaughters = p.children();
        if (bosonDaughters.size() == 2) {
          for (truth::Particle tau : bosonDaughters) {
            if (abs(tau.pdgId()) == 15) { // tau lepton
              const truth::Branch tauBranch(&graph, tau.id());
              int decayMode = getGenTauDecayMode(tauBranch);
              histograms_.at("genTauPt")->Fill(tau.momentum().pt());
              histograms_.at("genTauVisPt")->Fill(tauBranch.visibleP4().pt());
              histograms_.at("genTauDecay")->Fill(decayMode);
            }
          }
        }
      }
    }
  }

   for (unsigned itau = 0; itau < recoTaus.size(); ++itau) {
    const reco::PFTau& recoTau = recoTaus[itau];
    int decayMode = getPFTauDecayMode(recoTau);
    histograms_.at("recoTauPt")->Fill(recoTau.pt());
    histograms_.at("recoTauDecay")->Fill(decayMode);
  }
}

void DYToTauTauAnalyser::beginJob() {
  // register to the TFileService
  edm::Service<TFileService> fs;
  
  // create histograms
  // histograms_["GenZMass"] = fs->make<TH1F>("GenBosonMass", "m_{Z}", 50, 60, 120);
  // histograms_["GenZMass"]->GetXaxis()->SetTitle("m_{Z} [GeV]");
  // histograms_["GenZMass"]->GetYaxis()->SetTitle("Events");

  // histograms_["mTauTauVis"] = fs->make<TH1F>("mTauTauVis", "m_{#tau#tau}^{vis}", 50, 0, 300);
  // histograms_["mTauTauVis"]->GetXaxis()->SetTitle("m_{#tau#tau}^{vis} [GeV]");
  // histograms_["mTauTauVis"]->GetYaxis()->SetTitle("Events");

  // histograms_["pTmiss"] = fs->make<TH1F>("pTmiss", "p_{T}^{miss}", 50, 0, 75);
  // histograms_["pTmiss"]->GetXaxis()->SetTitle("p_{T}^{miss} [GeV]");
  // histograms_["pTmiss"]->GetYaxis()->SetTitle("Events");

  histograms_["genTauDecay"] = fs->make<TH1F>("genTauDecay", "Gen Tau Hadronic Decays", 6, 1.5, 7.5);
	histograms_["genTauDecay"]->GetYaxis()->SetTitle("Events");
	histograms_["genTauDecay"]->GetXaxis()->SetBinLabel(1, "h^{#pm}");
	histograms_["genTauDecay"]->GetXaxis()->SetBinLabel(2, "h^{#pm}#pi^{0}");
	histograms_["genTauDecay"]->GetXaxis()->SetBinLabel(3, "h^{#pm}#pi^{0}#pi^{0}");
	histograms_["genTauDecay"]->GetXaxis()->SetBinLabel(4, "h^{#mp}h^{#pm}h^{#pm}");
	histograms_["genTauDecay"]->GetXaxis()->SetBinLabel(5, "h^{#mp}h^{#pm}h^{#pm}#pi^{0}");
	histograms_["genTauDecay"]->GetXaxis()->SetBinLabel(6, "Other");

  histograms_["recoTauDecay"] = fs->make<TH1F>("recoTauDecay", "Reco Tau Hadronic Decays", 6, 1.5, 7.5);
  histograms_["recoTauDecay"]->GetYaxis()->SetTitle("Events");
  histograms_["recoTauDecay"]->GetXaxis()->SetBinLabel(1, "h^{#pm}");
  histograms_["recoTauDecay"]->GetXaxis()->SetBinLabel(2, "h^{#pm}#pi^{0}");
  histograms_["recoTauDecay"]->GetXaxis()->SetBinLabel(3, "h^{#pm}#pi^{0}#pi^{0}");
  histograms_["recoTauDecay"]->GetXaxis()->SetBinLabel(4, "h^{#mp}h^{#pm}h^{#pm}");
  histograms_["recoTauDecay"]->GetXaxis()->SetBinLabel(5, "h^{#mp}h^{#pm}h^{#pm}#pi^{0}");
  histograms_["recoTauDecay"]->GetXaxis()->SetBinLabel(6, "Other");

  histograms_["genTauPt"] = fs->make<TH1F>("genTauPt", "Gen Tau p_{T}", 50, 0, 100);
  histograms_["genTauPt"]->GetXaxis()->SetTitle("p_{T} [GeV]");
  histograms_["genTauPt"]->GetYaxis()->SetTitle("Events");

  histograms_["genTauVisPt"] = fs->make<TH1F>("genTauVisPt", "Gen Tau Visible p_{T}", 50, 0, 100);
  histograms_["genTauVisPt"]->GetXaxis()->SetTitle("p_{T}^{vis} [GeV]");
  histograms_["genTauVisPt"]->GetYaxis()->SetTitle("Events");

  histograms_["recoTauPt"] = fs->make<TH1F>("recoTauPt", "Reco Tau p_{T}", 50, 0, 100);
  histograms_["recoTauPt"]->GetXaxis()->SetTitle("p_{T} [GeV]");
  histograms_["recoTauPt"]->GetYaxis()->SetTitle("Events");
}

void DYToTauTauAnalyser::endJob() {
  std::cout << "Done!" << std::endl;
}

DEFINE_FWK_MODULE(DYToTauTauAnalyser);

