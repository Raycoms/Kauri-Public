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

    QuorumCertBLS::QuorumCertBLS(
            const ReplicaConfig &config, const uint256_t &obj_hash) :
            QuorumCert(), obj_hash(obj_hash), rids(config.nreplicas), t(config.nmajority){
        rids.clear();
    }

    bool QuorumCertBLS::verify(const ReplicaConfig &config) {
        if (theSig == nullptr) return false;
        HOTSTUFF_LOG_DEBUG("checking cert(%d), obj_hash=%s",
                           i, get_hex10(obj_hash).c_str());
        return theSig->verify(obj_hash, static_cast<const PubKeyBLS &>(*config.globalPub));
    }

    promise_t QuorumCertBLS::verify(const ReplicaConfig &config, VeriPool &vpool) {
        if (theSig == nullptr)
            return promise_t([](promise_t &pm) { pm.resolve(false); });

        std::vector<promise_t> vpm;

                HOTSTUFF_LOG_DEBUG("checking cert(%d), obj_hash=%s",
                                   i, get_hex10(obj_hash).c_str());
                vpm.push_back(vpool.verify(new SigVeriTaskBLS(obj_hash,
                                                                 static_cast<const PubKeyBLS &>(*config.globalPub),
                                                                 *theSig)));

        return promise::all(vpm).then([](const promise::values_t &values) {
            for (const auto &v: values)
                if (!promise::any_cast<bool>(v)) return false;
            return true;
        });
    }

    QuorumCertAggBLS::QuorumCertAggBLS(
            const ReplicaConfig &config, const uint256_t &obj_hash) :
            QuorumCert(), obj_hash(obj_hash), rids(config.nreplicas){
        rids.clear();
    }

    bool QuorumCertAggBLS::verify(const ReplicaConfig &config) {
        if (theSig == nullptr) return false;
        HOTSTUFF_LOG_DEBUG("checking cert(%d), obj_hash=%s",
                           i, get_hex10(obj_hash).c_str());

        theSig->data->SetAggregationInfo(bls::AggregationInfo::FromMsg(*pub, obj_hash.to_bytes().data(), obj_hash.to_bytes().size()));
        return theSig->data->Verify();
    }

    promise_t QuorumCertAggBLS::verify(const ReplicaConfig &config, VeriPool &vpool) {
        if (theSig == nullptr)
            return promise_t([](promise_t &pm) { pm.resolve(false); });
        std::vector<promise_t> vpm;

        struct timeval timeStart,
                timeEnd;
        gettimeofday(&timeStart, NULL);

        theSig->data->SetAggregationInfo(bls::AggregationInfo::FromMsg(*pub, obj_hash.to_bytes().data(), obj_hash.to_bytes().size()));
        bool veri = theSig->data->Verify();

        gettimeofday(&timeEnd, NULL);

        std::cout << "This verifying agg piece of code took "
                  << ((timeEnd.tv_sec - timeStart.tv_sec) * 1000000 + timeEnd.tv_usec - timeStart.tv_usec)
                  << " us to execute."
                  << std::endl;

        if (veri) {
            return promise_t([](promise_t &pm) { pm.resolve(true); });
        }
        return promise_t([](promise_t &pm) { pm.resolve(false); });
    }

    void QuorumCertAggBLS::add_part(const ReplicaConfig &config, ReplicaID rid, const PartCert &pc) {
        if (pc.get_obj_hash() != obj_hash)
            throw std::invalid_argument("PartCert does match the block hash");
        rids.set(rid);
        calculateN();

        if (theSig == nullptr) {
            theSig = new SigSecBLSAgg(*dynamic_cast<const PartCertBLSAgg &>(pc).data);
            pub = new bls::PublicKey(*dynamic_cast<const PubKeyBLS & > (config.get_pubkey(rid)).data);
            return;
        }
        uint8_t hash[bls::BLS::MESSAGE_HASH_LEN];
        bls::Util::Hash256(hash, obj_hash.to_bytes().data(),  obj_hash.to_bytes().size());

        bls::Signature sig1 = *theSig->data;
        sig1.SetAggregationInfo(bls::AggregationInfo::FromMsgHash(*pub, hash));

        bls::Signature sig2 = *dynamic_cast<const SigSecBLSAgg &>(pc).data;
        bls::PublicKey pub2 = *dynamic_cast<const PubKeyBLS & > (config.get_pubkey(rid)).data;
        sig2.SetAggregationInfo(bls::AggregationInfo::FromMsgHash(pub2, hash));



        struct timeval timeStart,
                timeEnd;
        gettimeofday(&timeStart, NULL);
        bls::Signature sig = bls::Signature::Aggregate({sig1, sig2});
        bls::PublicKey resultPub = bls::PublicKey::Aggregate({*pub, pub2});
        gettimeofday(&timeEnd, NULL);

        std::cout << "This aggregating slow piece of code took "
                  << ((timeEnd.tv_sec - timeStart.tv_sec) * 1000000 + timeEnd.tv_usec - timeStart.tv_usec)
                  << " us to execute."
                  << std::endl;

        *theSig->data = sig;
        *pub = resultPub;
    }
}
