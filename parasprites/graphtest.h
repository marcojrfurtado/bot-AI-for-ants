//#ifdef DEBUG
//
////For debugging
//static void printAnt(std::ostream& str, akey_t key, const AntNode& ant, const AntNode::State& moves){
//    str << key << ": " << ant.pos << " " << ant.neighbors.size() << " neighbors \n";
//
//    for(auto it = ant.neighbors.begin(); it != ant.neighbors.end(); ++it){
//        str << it->key << " " << it->mask << "\n";
//    }
//
//    for(int i=0; i<5; ++i) {
//        if (!moves[i]) {continue;}
//        str << "\t" << CDIRECTIONS[i] << ": " << ant.movedata[i].sqrkey << ", " << ant.movedata[i].fscore << "\n";
////        str << "\t" << CDIRECTIONS[i] << ": " << ant.movedata[i].sqrkey << ", " << ant.movedata[i].fscore << ", [";
////        for(auto it = ant.movedata[i].neighbors.begin(); it != ant.movedata[i].neighbors.end(); ++it){
////            str << "(" << it->key << ", " << it->mask << ")";
////            if (it+1 !=  ant.movedata[i].neighbors.end()) {str<<", ";}
////        }
////        str << "]\n";
//    }
//}
////
////static std::ostream& operator<<(std::ostream& str, const ConstraintGraph& graph){
////    const auto& gstate = graph.baseState();
////
////    str << "Ants: " << graph.ants.size() << "\n";
////
////    for(auto it = graph.ants.begin(); it != graph.ants.end(); ++it){
//////        ASSERT(gstate.ants.count(it->key));
////        printAnt(str, it->key, *it, gstate.ants.at(it->key) );
////    }
////
////    return str;
////}
//
//#endif
