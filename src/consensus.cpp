/**
 * Copyright 2018 VMware
 * Copyright 2018 Ted Yin
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

#include <cassert>
#include <stack>

#include "hotstuff/util.h"
#include "hotstuff/consensus.h"

#define LOG_INFO HOTSTUFF_LOG_INFO
#define LOG_DEBUG HOTSTUFF_LOG_DEBUG
#define LOG_WARN HOTSTUFF_LOG_WARN
#define LOG_PROTO HOTSTUFF_LOG_PROTO

namespace hotstuff {

/* The core logic of HotStuff, is fairly simple :). */
/*** begin HotStuff protocol logic ***/
HotStuffCore::HotStuffCore(ReplicaID id,
                            privkey_bt &&priv_key):
        b0(new Block(true, 1)),
        b_lock(b0),
        b_exec(b0),
        vheight(0),
        priv_key(std::move(priv_key)),
        tails{b0},
        vote_disabled(false),
        id(id),
        storage(new EntityStorage()) {
    storage->add_blk(b0);
}

void HotStuffCore::sanity_check_delivered(const block_t &blk) {
    if (!blk->delivered)
        throw std::runtime_error("block not delivered");
}

block_t HotStuffCore::get_potentially_not_delivered_blk(const uint256_t &blk_hash) {
    block_t blk = storage->find_blk(blk_hash);
    if (blk == nullptr)
        throw std::runtime_error("block not delivered " + std::to_string(blk == nullptr));
    return blk;
}

block_t HotStuffCore::get_delivered_blk(const uint256_t &blk_hash) {
    block_t blk = storage->find_blk(blk_hash);
    if (blk == nullptr || !blk->delivered) {
        HOTSTUFF_LOG_PROTO("block %s not delivered", get_hex10(blk_hash).c_str());
        throw std::runtime_error("block not delivered ");
    }
    return blk;
}

bool HotStuffCore::on_deliver_blk(const block_t &blk) {
    HOTSTUFF_LOG_PROTO("Core deliver");

    if (blk->delivered)
    {
        LOG_WARN("attempt to deliver a block twice");
        return false;
    }
    blk->parents.clear();
    for (const auto &hash: blk->parent_hashes) {
        if (b_piped != nullptr && hash == b_piped->hash) {
            blk->parents.push_back(b_piped);
        }
        else {
            blk->parents.push_back(get_delivered_blk(hash));
        }
    }
    blk->height = blk->parents[0]->height + 1;

    if (blk->qc)
    {
        block_t _blk = storage->find_blk(blk->qc->get_obj_hash());
        if (_blk == nullptr)
            throw std::runtime_error("block referred by qc not fetched");
        blk->qc_ref = std::move(_blk);
    } // otherwise blk->qc_ref remains null

    for (auto pblk: blk->parents) tails.erase(pblk);
    tails.insert(blk);

    blk->delivered = true;
    HOTSTUFF_LOG_PROTO("deliver %s", std::string(*blk).c_str());
    return true;
}

void HotStuffCore::update_hqc(const block_t &_hqc, const quorum_cert_bt &qc) {
    if (_hqc->height > hqc.first->height)
    {
        hqc = std::make_pair(_hqc, qc->clone());
        on_hqc_update();
    }
}

void HotStuffCore::update(const block_t &nblk) {
    /* nblk = b*, blk2 = b'', blk1 = b', blk = b */
#ifndef HOTSTUFF_TWO_STEP
    /* three-step HotStuff */
    const block_t &blk2 = nblk->qc_ref;
    if (blk2 == nullptr) return;
    /* decided blk could possible be incomplete due to pruning */
    if (blk2->decision) return;
    update_hqc(blk2, nblk->qc);

    const block_t &blk1 = blk2->qc_ref;
    if (blk1 == nullptr) return;
    if (blk1->decision) return;
    if (blk1->height > b_lock->height) b_lock = blk1;

    const block_t &blk = blk1->qc_ref;
    if (blk == nullptr) return;
    if (blk->decision) return;

    /* commit requires direct parent */
    if (blk2->parents[0] != blk1 || blk1->parents[0] != blk) return;
#else
    /* two-step HotStuff */
    const block_t &blk1 = nblk->qc_ref;
    if (blk1 == nullptr) return;
    if (blk1->decision) return;
    update_hqc(blk1, nblk->qc);
    if (blk1->height > b_lock->height) b_lock = blk1;

    const block_t &blk = blk1->qc_ref;
    if (blk == nullptr) return;
    if (blk->decision) return;

    /* commit requires direct parent */
    if (blk1->parents[0] != blk) return;
#endif
    /* otherwise commit */
    std::vector<block_t> commit_queue;
    block_t b;
    for (b = blk; b->height > b_exec->height; b = b->parents[0])
    {
        commit_queue.push_back(b);
    }
    if (b != b_exec)
        throw std::runtime_error("safety breached :( " +
                                std::string(*blk) + " " +
                                std::string(*b_exec));
    for (auto it = commit_queue.rbegin(); it != commit_queue.rend(); it++)
    {
        const block_t &blk = *it;
        blk->decision = 1;
        do_consensus(blk);
        LOG_PROTO("commit %s", std::string(*blk).c_str());
        for (size_t i = 0; i < blk->cmds.size(); i++)
            do_decide(Finality(id, 1, i, blk->height,
                                blk->cmds[i], blk->get_hash()));
    }
    b_exec = blk;
}

block_t HotStuffCore::on_propose(const std::vector<uint256_t> &cmds,
                            const std::vector<block_t> &parents,
                            bytearray_t &&extra) {
    if (parents.empty())
        throw std::runtime_error("empty parents");
    for (const auto &_: parents) tails.erase(_);
    /* create the new block */

    block_t bnew;
    if (b_piped == nullptr) {
        LOG_PROTO("b_piped is null");
        bnew = storage->add_blk(
                new Block(parents, cmds,
                          hqc.second->clone(), std::move(extra),
                          parents[0]->height + 1,
                          hqc.first,
                          nullptr
                ));
    } else {
        auto newParents = std::vector<block_t>(parents);

        LOG_PROTO("b_piped is not null");
        newParents.insert(newParents.begin(), b_piped);
        //todo: On normal propose (below) we have to check if we have a pending b_piped block with a higher height and then adjust the block we send out for that
        bnew = storage->add_blk(
                new Block(newParents, cmds,
                          hqc.second->clone(), std::move(extra),
                          newParents[0]->height + 1,
                          b_piped,
                          nullptr
                ));
    }

    LOG_PROTO("propose %s", std::string(*bnew).c_str());
    Proposal prop = process_block(bnew, true);
    /* broadcast to other replicas */
    do_broadcast_proposal(prop);

    return bnew;
}

Proposal HotStuffCore::process_block(const block_t& bnew, bool adjustHeight)
{
    const uint256_t bnew_hash = bnew->get_hash();
    if (bnew->self_qc == nullptr) {
        bnew->self_qc = create_quorum_cert(bnew_hash);
    }

    on_deliver_blk(bnew);
    LOG_PROTO("before update");
    update(bnew);
    Proposal prop(id, bnew, nullptr);
    //std::cout << "prop" << std::endl;
    /* self-vote */
    if (adjustHeight) {
        if (bnew->height <= vheight)
            throw std::runtime_error("new block should be higher than vheight");
        vheight = bnew->height;
    }

    if (storage->find_blk(bnew_hash) == nullptr) {
        LOG_PROTO("not in storage!");
    }

    LOG_PROTO("before On receive vote");

    on_receive_vote(
            Vote(id, bnew_hash,
                 create_part_cert(*priv_key, bnew_hash), this));
    LOG_PROTO("after On receive vote");
    on_propose_(prop);
    LOG_PROTO("after on propose");

    return prop;
}

void HotStuffCore::on_receive_proposal(const Proposal &prop) {
    LOG_PROTO("got %s", std::string(prop).c_str());
    block_t bnew = prop.blk;
    sanity_check_delivered(bnew);
    update(bnew);
    bool opinion = false;
    if (bnew->height > vheight)
    {
        if (bnew->qc_ref && bnew->qc_ref->height > b_lock->height)
        {
            opinion = true; // liveness condition
            vheight = bnew->height;
        }
        else
        {   // safety condition (extend the locked branch)
            block_t b;
            for (b = bnew;
                b->height > b_lock->height;
                b = b->parents[0]);
            if (b == b_lock) /* on the same branch */
            {
                opinion = true;
                vheight = bnew->height;
            }
        }
    }

    LOG_PROTO("x now state: %s", std::string(*this).c_str());
    if (bnew->qc_ref) {
        on_qc_finish(bnew->qc_ref);
    }

    on_receive_proposal_(prop);
    if (opinion && !vote_disabled) {
        std::cout << "Propose handler4 - do vote" << std::endl;

        do_vote(prop,
                Vote(id, bnew->get_hash(),
                     create_part_cert(*priv_key, bnew->get_hash()), this));
    }
}

void HotStuffCore::on_receive_vote(const Vote &vote) {
    LOG_PROTO("got %s", std::string(vote).c_str());
    LOG_PROTO("y now state: %s", std::string(*this).c_str());

    block_t blk = get_delivered_blk(vote.blk_hash);
    assert(vote.cert);

    LOG_WARN("after delivered check!");

    if (!blk->voted.insert(vote.voter).second)
    {
        LOG_WARN("duplicate vote for %s from %d", get_hex10(vote.blk_hash).c_str(), vote.voter);
        return;
    }

    if (vote.voter != get_id()) return;
    if (blk->qc != nullptr && blk->qc->has_n(config.nmajority)) return;

    //std::cout << "self vote" << std::endl;
    auto &qc = blk->self_qc;
    if (qc == nullptr)
    {
        LOG_WARN("vote for block not proposed by itself");
        qc = create_quorum_cert(blk->get_hash());
    }

    qc->add_part(config, vote.voter, *vote.cert);
    if (qc->has_n(config.nmajority))
    {
        qc->compute();
        update_hqc(blk, qc);
        on_qc_finish(blk);
    }
}

/*** end HotStuff protocol logic ***/
void HotStuffCore::on_init(uint32_t nfaulty) {

    config.nmajority = config.nreplicas - nfaulty;
    b0->qc = create_quorum_cert(b0->get_hash());
    //b0->qc->compute();
    b0->self_qc = b0->qc->clone();
    b0->qc_ref = b0;
    hqc = std::make_pair(b0, b0->qc->clone());
}

void HotStuffCore::prune(uint32_t staleness) {
    block_t start;
    /* skip the blocks */
    for (start = b_exec; staleness; staleness--, start = start->parents[0])
        if (!start->parents.size()) return;
    std::stack<block_t> s;
    start->qc_ref = nullptr;
    s.push(start);
    while (!s.empty())
    {
        auto &blk = s.top();
        if (blk->parents.empty())
        {
            storage->try_release_blk(blk);
            s.pop();
            continue;
        }
        blk->qc_ref = nullptr;
        s.push(blk->parents.back());
        blk->parents.pop_back();
    }
}

void HotStuffCore::add_replica(ReplicaID rid, const PeerId &peer_id,
                                pubkey_bt &&pub_key) {
    config.add_replica(rid,
            ReplicaInfo(rid, peer_id, std::move(pub_key)));
    b0->voted.insert(rid);
}

promise_t HotStuffCore::async_qc_finish(const block_t &blk) {
    //std::cout << "test " << blk->voted.size() << " " << blk->self_qc->has_n(config.nmajority) << std::endl;

    if ((blk->self_qc != nullptr && blk->self_qc->has_n(config.nmajority) && !blk->voted.empty()) || blk->voted.size() >= config.nmajority)
        return promise_t([](promise_t &pm) {
            pm.resolve();
        });
    auto it = qc_waiting.find(blk);
    if (it == qc_waiting.end())
        it = qc_waiting.insert(std::make_pair(blk, promise_t())).first;
    return it->second;
}

void HotStuffCore::on_qc_finish(const block_t &blk) {
    auto it = qc_waiting.find(blk);
    if (it != qc_waiting.end())
    {
        it->second.resolve();
        qc_waiting.erase(it);
    }
}

promise_t HotStuffCore::async_wait_proposal() {
    return propose_waiting.then([](const Proposal &prop) {
        return prop;
    });
}

promise_t HotStuffCore::async_wait_receive_proposal() {
    return receive_proposal_waiting.then([](const Proposal &prop) {
        return prop;
    });
}

promise_t HotStuffCore::async_hqc_update() {
    return hqc_update_waiting.then([this]() {
        return hqc.first;
    });
}

void HotStuffCore::on_propose_(const Proposal &prop) {
    auto t = std::move(propose_waiting);
    propose_waiting = promise_t();
    t.resolve(prop);
}

void HotStuffCore::on_receive_proposal_(const Proposal &prop) {
    auto t = std::move(receive_proposal_waiting);
    receive_proposal_waiting = promise_t();
    t.resolve(prop);
}

void HotStuffCore::on_hqc_update() {
    auto t = std::move(hqc_update_waiting);
    hqc_update_waiting = promise_t();
    t.resolve();
}

HotStuffCore::operator std::string () const {
    DataStream s;
    s << "<hotstuff "
      << "hqc=" << get_hex10(hqc.first->get_hash()) << " "
      << "hqc.height=" << std::to_string(hqc.first->height) << " "
      << "b_lock=" << get_hex10(b_lock->get_hash()) << " "
      << "b_exec=" << get_hex10(b_exec->get_hash()) << " "
      << "vheight=" << std::to_string(vheight) << " "
      << "tails=" << std::to_string(tails.size()) << ">";
    return s;
}

void HotStuffCore::set_fanout(int32_t fanout) {
    config.fanout = fanout;
}

void HotStuffCore::set_piped_latency(int32_t piped_latency) {
    config.piped_latency = piped_latency;
}

}
