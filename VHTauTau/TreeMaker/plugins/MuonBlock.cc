#include <iostream>

#include "TTree.h"
#include "TClonesArray.h"

#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "DataFormats/PatCandidates/interface/Muon.h"
#include "DataFormats/TrackReco/interface/HitPattern.h"
#include "DataFormats/BeamSpot/interface/BeamSpot.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "Math/GenVector/VectorUtil.h"

#include "DataFormats/GeometryVector/interface/GlobalPoint.h"
#include "DataFormats/GeometryVector/interface/VectorUtil.h"

#include "VHTauTau/TreeMaker/plugins/MuonBlock.h"
#include "VHTauTau/TreeMaker/interface/Utility.h"


const reco::GenParticle* getGenMuon(const pat::Muon& pm)
{
  std::vector<reco::GenParticleRef> associatedGenParticles = pm.genParticleRefs();
  for ( std::vector<reco::GenParticleRef>::const_iterator im = associatedGenParticles.begin(); 
	im != associatedGenParticles.end(); ++im ) {
    if ( im->isAvailable() ) {
      const reco::GenParticleRef& genParticle = (*im);
      if ( genParticle->pdgId() == -13 || genParticle->pdgId() == +13 ) return genParticle.get();
    }
  }
  return 0;
}

const reco::GenParticle* findMotherWithPdgId(const reco::GenParticle* genParticle, unsigned absPdgId)
{
  // If this doesn't have any valid mother, return null
  const reco::GenParticle* retVal = 0;
  
  if ( genParticle ) {
    // Get list of mothers. Not sure why this would be more than one.
    size_t nGenMothers = genParticle->numberOfMothers();
    for ( size_t iGenMother = 0; iGenMother < nGenMothers; ++iGenMother ) {
      const reco::GenParticle* genMother = dynamic_cast<const reco::GenParticle*>(genParticle->mother(iGenMother));
      if ( genMother ) {
	unsigned genMotherPdgId = TMath::Abs(genMother->pdgId());
	if ( genMotherPdgId == absPdgId ) {
	  retVal = genMother;
        } else {
	  const reco::GenParticle* genGrandMother = findMotherWithPdgId(genMother, absPdgId);
	  if ( genGrandMother != 0 ) retVal = genGrandMother;
	}
      }

      if ( retVal != 0 ) break;
    }
  }

  return retVal;
}




MuonBlock::MuonBlock(const edm::ParameterSet& iConfig) :
  _verbosity(iConfig.getParameter<int>("verbosity")),
  _muonInputTag(iConfig.getParameter<edm::InputTag>("muonSrc")),
  _vtxInputTag(iConfig.getParameter<edm::InputTag>("vertexSrc")),
  _beamSpotInputTag(iConfig.getParameter<edm::InputTag>("offlineBeamSpot")),
  _beamSpotCorr(iConfig.getParameter<bool>("beamSpotCorr")),
 _muonID(iConfig.getParameter<std::string>("muonID"))
{
}
MuonBlock::~MuonBlock() {
}
void MuonBlock::beginJob() 
{
  // Get TTree pointer
  TTree* tree = vhtm::Utility::getTree("vhtree");
  cloneMuon = new TClonesArray("vhtm::Muon");
  tree->Branch("Muon", &cloneMuon, 32000, 2);
  tree->Branch("nMuon", &fnMuon,  "fnMuon/I");
}
void MuonBlock::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
  // Reset the TClonesArray and the nObj variables
  cloneMuon->Clear();
  fnMuon = 0;

  edm::Handle<std::vector<pat::Muon> > muons;
  iEvent.getByLabel(_muonInputTag, muons);

  edm::Handle<reco::VertexCollection> primaryVertices;
  iEvent.getByLabel(_vtxInputTag, primaryVertices);

  edm::Handle<reco::BeamSpot> beamSpot;
  iEvent.getByLabel(_beamSpotInputTag, beamSpot);

  if (muons.isValid()) {
    
    
    edm::LogInfo("MuonBlock") << "Total # Muons: " << muons->size();
    for (std::vector<pat::Muon>::const_iterator it  = muons->begin(); 
                                                it != muons->end(); ++it) {
      // consider only global muons
      //if (muons->size() == 2){
	//std::cout<<muons->size()<<std::endl;
      if (!it->isGlobalMuon()) continue;
      
      

      
      //remove those patmuons which were not matched with gen particles
      //const reco::GenParticle* genMuon = getGenMuon(*it);
      //if (!genMuon) continue;

      // Choose only those pat muons which have taus as mother particles
      
      const reco::GenParticle* genTau = findMotherWithPdgId(it->genLepton(), 15);
      if(!genTau) continue;
      
      
      if (fnMuon == kMaxMuon) {
	edm::LogInfo("MuonBlock") << "Too many PAT Muons, fnMuon = " << fnMuon; 
	break;
      }

      reco::TrackRef tk  = it->innerTrack();  // tracker segment only
      reco::TrackRef gtk = it->globalTrack(); 

      muonB = new ((*cloneMuon)[fnMuon++]) vhtm::Muon();
      muonB->isTrackerMuon = (it->isTrackerMuon()) ? true : false;
      muonB->isPFMuon = it->userInt("isPFMuon") ? true :  false; 
      
      //kinematic variables for the gen lepton
      muonB->ptgen   = it->genLepton()->pt(); 

      
      muonB->eta     = it->eta();
      muonB->phi     = it->phi();
      muonB->pt      = it->pt();
      muonB->ptError = tk->ptError();
      muonB->px      = it->px();
      muonB->py      = it->py();
      muonB->pz      = it->pz(); 
      muonB->energy  = it->energy();
      muonB->charge  = it->charge();

      double trkd0 = tk->d0();
      double trkdz = tk->dz();
      if (_beamSpotCorr) {
        if (beamSpot.isValid()) {
          trkd0 = -(tk->dxy(beamSpot->position()));
          trkdz = tk->dz(beamSpot->position());
        }
        else
          edm::LogError("MuonsBlock") << "Error >> Failed to get BeamSpot for label: "
                                      << _beamSpotInputTag;
      }
      muonB->trkD0      = trkd0;
      muonB->trkD0Error = tk->d0Error();
      muonB->trkDz      = trkdz;
      muonB->trkDzError = tk->dzError();
      muonB->globalChi2 = it->normChi2();
      muonB->passID     = (it->muonID(_muonID)) ? true : false;

      // Vertex association
      double minVtxDist3D = 9999.;
         int indexVtx = -1;
      double vertexDistZ = 9999.;
      if (primaryVertices.isValid()) {
	edm::LogInfo("MuonBlock") << "Total # Primary Vertices: " << primaryVertices->size();

        for (reco::VertexCollection::const_iterator vit  = primaryVertices->begin(); 
                                                    vit != primaryVertices->end(); ++vit) {

          double dxy = tk->dxy(vit->position());
          double dz  = tk->dz(vit->position());
          double dist3D = std::sqrt(pow(dxy,2) + pow(dz,2));
          if (dist3D < minVtxDist3D) {
            minVtxDist3D = dist3D;
            indexVtx     = int(std::distance(primaryVertices->begin(), vit));
            vertexDistZ  = dz;
          }
        }
      } 
      else {
	edm::LogError("MuonBlock") << "Error >> Failed to get VertexCollection for label: " 
                                   << _vtxInputTag;
      }
      muonB->vtxDist3D = minVtxDist3D;
      muonB->vtxIndex  = indexVtx;
      muonB->vtxDistZ  = vertexDistZ;

 
    // Hit pattern
      const reco::HitPattern& hitp = gtk->hitPattern();  // innerTrack will not provide Muon Hits 
      muonB->pixHits = hitp.numberOfValidPixelHits();
      muonB->trkHits = hitp.numberOfValidTrackerHits();
      muonB->muoHits = hitp.numberOfValidMuonHits();
      muonB->matches = it->numberOfMatches();
      muonB->trackerLayersWithMeasurement = hitp.trackerLayersWithMeasurement();

      // Isolation
      muonB->trkIso  = it->trackIso();
      muonB->ecalIso = it->ecalIso();
      muonB->hcalIso = it->hcalIso();
      muonB->hoIso   = it->isolationR03().hoEt;
      double reliso  = (it->trackIso() + it->ecalIso() + it->hcalIso())/it->pt();
      muonB->relIso  = reliso;

      muonB->pfRelIso = it->userFloat("PFRelIsoDB04v2");

      // IP information
      muonB->dB  = it->dB(pat::Muon::PV2D);
      muonB->edB = it->edB(pat::Muon::PV2D);
      //  muonB->dB_BS  = it->dB(pat::Muon::BS2D);
    
      if (primaryVertices.isValid()) {
	std::cout<<" db  "<<it->dB(pat::Muon::PV2D)<<" db with bs  "<<it->dB(pat::Muon::BS2D)<<std::endl;
	//	std::cout<<"  dxy  "<<it->globalTrack()->dxy((primaryVertices->begin())->position())<<std::endl;
      }


      muonB->dB3d  = it->dB(pat::Muon::PV3D);
      muonB->edB3d = it->edB(pat::Muon::PV3D);

      // UW recommendation
      muonB->isGlobalMuonPromptTight = muon::isGoodMuon(*it, muon::GlobalMuonPromptTight);
      muonB->isAllArbitrated         = muon::isGoodMuon(*it, muon::AllArbitrated);
      muonB->nChambers               = it->numberOfChambers();
      muonB->nMatches                = it->numberOfMatches();
      muonB->nMatchedStations        = it->numberOfMatchedStations();
      muonB->stationMask             = it->stationMask();
      muonB->stationGapMaskDistance  = it->stationGapMaskDistance();
      muonB->stationGapMaskPull      = it->stationGapMaskPull();
      muonB->muonID                  = it->userInt("muonID");

      // Vertex information
      const reco::Candidate::Point& vertex = it->vertex();
      muonB->vx = vertex.x();             
      muonB->vy = vertex.y();             
      muonB->vz = vertex.z();             

      muonB->idMVA = it->userFloat("muonIdMVA");
      muonB->isoRingsMVA = it->userFloat("muonIsoRingsMVA");
      muonB->isoRingsRadMVA = it->userFloat("muonIsoRingsRadMVA");
      muonB->idIsoCombMVA = it->userFloat("muonIdIsoCombMVA");

      muonB->pfRelIso03v1 = it->userFloat("PFRelIso03v1");
      muonB->pfRelIso03v2 = it->userFloat("PFRelIso03v2");
      muonB->pfRelIsoDB03v1 = it->userFloat("PFRelIsoDB03v1");
      muonB->pfRelIsoDB03v2 = it->userFloat("PFRelIsoDB03v2");

      muonB->pfRelIso04v1 = it->userFloat("PFRelIso04v1");
      muonB->pfRelIso04v2 = it->userFloat("PFRelIso04v2");
      muonB->pfRelIsoDB04v1 = it->userFloat("PFRelIsoDB04v1");
      muonB->pfRelIsoDB04v2 = it->userFloat("PFRelIsoDB04v2");
      }
    
} 
  else {
    edm::LogError("MuonBlock") << "Error >> Failed to get pat::Muon collection for label: " 
                               << _muonInputTag;
  }
}
#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(MuonBlock);
