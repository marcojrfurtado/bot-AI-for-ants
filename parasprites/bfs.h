#pragma once

void expandBFS(const State& state, Array2d<unsigned int>& dists, std::deque<Pos>& q, Pos node){
    const auto depth = dists[node];

    for(int d=0; d<4; d++)
    {
        Pos next = state.getDest(node, d);
        if (state.symmetry->isWater(next)) {continue;}
        if (dists[next] <= depth + 1) {continue;} //Already visited
        dists[next] = depth + 1;
        q.push_back(next);
    }
}
