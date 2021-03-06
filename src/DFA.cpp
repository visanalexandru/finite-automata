//
// Created by visan on 3/28/22.
//

#include "DFA.h"
#include<iostream>

void DFA::add_edge(state a, state b, letter s) {
    //Check to see if an edge with the same letter has already been used.
    for (Edge &edge: graph[a]) {
        if (edge.transition == s) {
            edge.next = b;
            return;
        }
    }
    //If the check did not return, just add the edge.
    graph[a].push_back({b, s});
    transposed[b].push_back(a);
}

std::set<state> DFA::alive_states() const {
    std::deque<state> current;
    std::set<state> reachable;

    //Run a multiple-root bfs to find the states that are reachable from any final state.
    for (state x: final) {
        current.push_back(x);
        reachable.insert(x);
    }

    while (!current.empty()) {
        state here = current.front();
        current.pop_front();

        if (transposed.find(here) == transposed.end()) {
            continue;
        }

        const auto &vec = transposed.find(here)->second;
        for (state next: vec) {
            if (reachable.find(next) == reachable.end()) {
                reachable.insert(next);
                current.push_back(next);
            }
        }
    }
    return reachable;
}

std::set<state> DFA::reachable_states() const {
    std::deque<state> current = {start};
    std::set<state> reachable = {start};

    //Run a single-root bfs to find the states that are reachable from the start state.
    while (!current.empty()) {
        state here = current.front();
        current.pop_front();

        if (graph.find(here) == graph.end()) {
            continue;
        }

        const auto &vec = graph.find(here)->second;
        for (const Edge &edge: vec) {
            if (reachable.find(edge.next) == reachable.end()) {
                reachable.insert(edge.next);
                current.push_back(edge.next);
            }
        }
    }

    return reachable;
}

std::set<state> DFA::valid_states() const {
    return intersection(alive_states(), reachable_states());
}

bool DFA::valid(const std::string &word) const {
    state current = start;

    for (letter x: word) {

        if (graph.find(current) == graph.end()) {
            return false;
        }

        const auto &vec = graph.find(current)->second;
        bool found = false;

        //Iterate through all the edges of the current state, see if an edge with the current symbol exists.
        for (const Edge &edge: vec) {
            if (edge.transition == x) {
                found = true;
                current = edge.next;
                break;
            }
        }

        //If we didn't find any edge for the current symbol, reject the word.
        if (!found) {
            return false;
        }
    }

    // If the current state is final, the word is accepted.
    if (final.find(current) != final.end()) {
        return true;
    }
    return false;
}


DFA DFA::minimize() const {
    std::set<state> valid = valid_states();

    //Implementation of hopcroft's algorithm
    std::set<std::set<state>> w;
    w.insert(intersection(final, valid));
    w.insert(intersection(non_final(), valid));
    std::set<std::set<state>> p = w;

    std::set<letter> letters = alphabet();

    //while W is not empty do
    while (!w.empty()) {
        //choose and remove a set A from W
        std::set<state> a = *w.begin();
        w.erase(w.begin());

        //for each c in the alphabet do
        for (letter c: letters) {
            //let X be the set of states for which a transition on c leads to a state in A
            std::set<state> x;
            for (const auto &node: graph) {

                //Only consider nodes that are valid.
                if (valid.find(node.first) == valid.end())
                    continue;

                for (const Edge &edge: node.second) {
                    if (edge.transition == c && a.find(edge.next) != a.end()) {
                        x.insert(node.first);
                        break;
                    }
                }
            }

            std::set<std::set<state>> old_p = p;
            //for each set Y in P for which X intersected with Y is nonempty and Y minus X is nonempty do
            for (const auto &y: p) {
                std::set<state> inter = intersection(x, y);
                std::set<state> diff = difference(y, x);
                if (inter.empty() || diff.empty())
                    continue;
                //replace Y in P by the two sets X intersected with Y and Y minus X
                old_p.erase(y);
                old_p.insert(inter);
                old_p.insert(diff);
                //if Y is in W
                if (w.find(y) != w.end()) {
                    //replace Y in W by the same two sets
                    w.erase(y);
                    w.insert(inter);
                    w.insert(diff);
                } else {
                    //if |intersection| <= |difference|
                    if (inter.size() <= diff.size()) {
                        //add intersection to W
                        w.insert(inter);
                    } else {
                        //add difference to W
                        w.insert(diff);
                    }
                }
            }
            p = old_p;
        }
    }
    //Map each state to the partition it is in.
    std::map<state, size_t> partitions;
    size_t partition_index = 0;
    for (const auto &s: p) {
        for (state x: s) {
            partitions[x] = partition_index;
        }
        partition_index++;
    }

    DFA result;
    partition_index = 0;

    for (const auto &s: p) {
        for (state x: s) {
            //If the state is final, the partition that contains it is also final.
            if (final.find(x) != final.end()) {
                result.set_final(partition_index);
            }

            //If the state is initial, the partition that contains it is also initial.
            if (x == start) {
                result.set_initial(partition_index);
            }

            if (graph.find(x) == graph.end())
                continue;

            const auto &vec = graph.find(x)->second;
            //For each edge from this state, add edges to the partition containing the neighbouring state.
            for (const Edge &edge: vec) {
                //Only consider states that are valid.
                if (valid.find(edge.next) == valid.end())
                    continue;
                result.add_edge(partition_index, partitions[edge.next], edge.transition);
            }
        }
        partition_index++;
    }

    return result;
}