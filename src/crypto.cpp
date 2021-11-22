/**
 * Copyright 2018 VMware
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hotstuff/entity.h"
#include "hotstuff/crypto.h"

namespace hotstuff {

    vector<uint8_t> arrToVec(const bytearray_t &arr)
    {
        return std::vector<uint8_t>(arr.begin(), arr.end());
    }

    secp256k1_context_t secp256k1_default_sign_ctx = new Secp256k1Context(true);
    secp256k1_context_t secp256k1_default_verify_ctx = new Secp256k1Context(false);

    QuorumCertSecp256k1::QuorumCertSecp256k1(
            const ReplicaConfig &config, const uint256_t &obj_hash) :
            QuorumCert(), obj_hash(obj_hash), rids(config.nreplicas) {
        rids.clear();
    }

    bool QuorumCertSecp256k1::verify(const ReplicaConfig &config) {
        //todo the sig sizes don't work! We might want to remove this and test, but gotta make sure we don't break it and make it easier.
        //if (sigs.size() < config.nmajority) return false;
        for (size_t i = 0; i < rids.size(); i++)
            if (rids.get(i)) {
                HOTSTUFF_LOG_DEBUG("checking cert(%d), obj_hash=%s",
                                   i, get_hex10(obj_hash).c_str());
                if (!sigs[i].verify(obj_hash,
                                    static_cast<const PubKeySecp256k1 &>(config.get_pubkey(i)),
                                    secp256k1_default_verify_ctx))
                    return false;
            }
        return true;
    }

    promise_t QuorumCertSecp256k1::verify(const ReplicaConfig &config, VeriPool &vpool) {
        //if (sigs.size() < config.nmajority)
            //return promise_t([](promise_t &pm) { pm.resolve(false); });
        std::vector<promise_t> vpm;
        for (size_t i = 0; i < rids.size(); i++)
            if (rids.get(i)) {
                HOTSTUFF_LOG_DEBUG("checking cert(%d), obj_hash=%s",
                                   i, get_hex10(obj_hash).c_str());
                vpm.push_back(vpool.verify(new Secp256k1VeriTask(obj_hash,
                                                                 static_cast<const PubKeySecp256k1 &>(config.get_pubkey(
                                                                         i)),
                                                                 sigs[i])));
            }
        return promise::all(vpm).then([](const promise::values_t &values) {
            for (const auto &v: values)
                if (!promise::any_cast<bool>(v)) return false;
            return true;
        });
    }

    QuorumCertAggBLS::QuorumCertAggBLS(const ReplicaConfig &config, const uint256_t &obj_hash) :QuorumCert(), obj_hash(obj_hash), rids(config.nreplicas){
        rids.clear();
    }

    /**
     * Verify the aggregated signature.
     */
    bool QuorumCertAggBLS::verify(const ReplicaConfig &config) {
      compute();
      if (theSig == nullptr) {
        return false;
      }

        vector<bls::G1Element> pubs;
        for (unsigned int i = 0; i < rids.size(); i++) {
            if (rids[i] == 1) {
                pubs.push_back(*dynamic_cast<const PubKeyBLS &>(config.get_pubkey(i)).data);
            }
        }

        bool res = bls::PopSchemeMPL::FastAggregateVerify(pubs, arrToVec(obj_hash.to_bytes()), *theSig->data);

        if (res || sigs.empty()) {
          sigs.clear();
          return res;
        }

        HOTSTUFF_LOG_PROTO("Signature Invalid, backing up!");
        vector<std::pair<salticidae::Bits, bls::G2Element>> newSigs;
        vector<bls::G1Element> newPubVec;

        for (const std::pair<salticidae::Bits, bls::G2Element> &sig : sigs) {

          vector<bls::G1Element> newPubs;
          for (unsigned int i = 0; i < rids.size(); i++) {
            if (rids[i] == 1) {
              newPubs.push_back(*dynamic_cast<const PubKeyBLS &>(config.get_pubkey(i)).data);
            }
          }

          if (bls::PopSchemeMPL::FastAggregateVerify(newPubs, arrToVec(obj_hash.to_bytes()), sig.second)) {
            newSigs.push_back(sig);
            for (bls::G1Element p : newPubs) {
              newPubVec.push_back(p);
            }
          }
        }
        sigs = std::move(newSigs);
        compute();
        return bls::PopSchemeMPL::FastAggregateVerify(newPubVec, arrToVec(obj_hash.to_bytes()), *theSig->data);
    }

    /**
     * Add another partial signature to the quorum sig.
     */
    void QuorumCertAggBLS::add_part(const ReplicaConfig &config, ReplicaID rid, const PartCert &pc) {

      if (pc.get_obj_hash() != obj_hash)
        throw std::invalid_argument("PartCert does match the block hash");

      salticidae::Bits saveRids(config.nreplicas);
      saveRids.set(rid);

      if (sigs.empty() && theSig != nullptr) {
        sigs.emplace_back(rids, *theSig->data);
        delete theSig;
        theSig = nullptr;
      }

      rids.set(rid);
      calculateN();

      sigs.emplace_back(saveRids, *dynamic_cast<const SigSecBLSAgg &>(pc).data);
    }

    /**
     * Merge two quorum signatures.
     */
    void QuorumCertAggBLS::merge_quorum(const QuorumCert &qc) {
      if (qc.get_obj_hash() != obj_hash)
        throw std::invalid_argument("QuorumCert does match the block hash");

      salticidae::Bits newRids =dynamic_cast<const QuorumCertAggBLS &>(qc).rids;
      for (unsigned int i = 0; i < newRids.size(); i++) {
        if (newRids[i] == 1) {
          rids.set(i);
        }
      }
      calculateN();

      if (sigs.empty() && theSig != nullptr) {
        sigs.emplace_back(std::move(rids), *theSig->data);
        delete theSig;
        theSig = nullptr;
      }

      for (const std::pair<salticidae::Bits, bls::G2Element>& el :dynamic_cast<const QuorumCertAggBLS &>(qc).sigs) {
        sigs.push_back(el);
      }

      if (dynamic_cast<const QuorumCertAggBLS &>(qc).theSig != nullptr) {
        sigs.emplace_back(std::move(dynamic_cast<const QuorumCertAggBLS &>(qc).rids), *dynamic_cast<const QuorumCertAggBLS &>(qc).theSig->data);
      }
    }

    promise_t QuorumCertAggBLS::verify(const ReplicaConfig &config,
                                       VeriPool &vpool) {
      std::vector<promise_t> vpm;

      vector<bls::G1Element> pubs;
      for (unsigned int i = 0; i < rids.size(); i++) {
        if (rids[i] == 1) {
          pubs.push_back(
              *dynamic_cast<const PubKeyBLS &>(config.get_pubkey(i)).data);
        }
      }

      vpm.push_back(vpool.verify(new SigVeriTaskBLSAgg(*this, pubs)));

      return promise::all(vpm).then([](const promise::values_t &values) {
        for (const auto &v : values)
          if (!promise::any_cast<bool>(v))
            return false;
        return true;
      });
    }
}
