#include "search.h"
#include <sstream>

namespace ZeroLogic {
	namespace Time {

		std::chrono::steady_clock::time_point stop_at;
		std::chrono::steady_clock::time_point started_at;

		void start_time(int msec) {
			stop_at = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(msec - 1);
			started_at = std::chrono::high_resolution_clock::now();
		}

		bool timed_out() {
			if (std::chrono::high_resolution_clock::now() >= stop_at) {
				return true;
			}
			else {
				return false;
			}
		}

		int getMsec() {
			return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - started_at).count() * 10e-7;
		}

	}
}

using namespace ZeroLogic;
using namespace Search;
using namespace Evaluation;

int Stats::partialnodes = 0;
uint32_t Stats::overallnodes = 0;
int Stats::fullDepth = 0;
int Stats::maxDepth = 0;


SearchInstance::SearchInstance(Gamestate* externalGamestate) {
	gs = (*externalGamestate);
}

int SearchInstance::initBestmove(int depth, int alpha, int beta, std::stack<Move> pv, std::stack<Move>* lowerPv) {

	std::stack<Move>* newPv = &pv;

	if (pv.empty()) { newPv = nullptr; }

	Movegenerator mg(&gs, movelist, &movenum);
	mg.generate();

	if (*movelist == checkmate) { return 0; }
	else if (*movelist == stalemate) { return 0; }

	sort(newPv);

	int eval = 0;

	for (int i = 0; i < movenum; ++i) {

		if (Time::timed_out()) { return 0; }

		std::stack<Move> higherPv;

		SearchInstance SI(&gs);
		eval = -SI.bestmove(sortedMovelist[i], depth - 1, -beta, -alpha, newPv, &higherPv);
		if (eval >= beta) { return beta; }
		if (eval > alpha) {
			alpha = eval;
			higherPv.push(sortedMovelist[i]);
			(*lowerPv) = higherPv;
		}
	}

	return alpha;

}

void SearchInstance::initID(int depth) {

	Stats::overallnodes = 0;

	int windowlow = 50;
	int windowhigh = 50;

	Move Bestmove{};
	int currentIterationEval{};
	int nextDepth = 1;
	std::stack<Move> newPv{};
	std::stack<Move> lastPv{};
	std::stack<Move> displayPv{};

	Misc misc;

	while (depth) {

		Stats::fullDepth = nextDepth;

		SearchInstance SI(&gs);
		currentIterationEval = SI.initBestmove(nextDepth, lastIterationEval - windowlow, lastIterationEval + windowhigh, lastPv, &newPv);
		Stats::maxDepth = (Stats::maxDepth) ? Stats::maxDepth : nextDepth;

		std::stringstream ss;

		ss << "info depth " << nextDepth << " nodes " << Stats::overallnodes << " seldepth " << Stats::maxDepth;
		ss << " time " << Time::getMsec();
		if (abs(currentIterationEval) > 90000) {
			if (currentIterationEval > 0) {
				ss << " score mate " << (((100000 - currentIterationEval) % 2) ? ((100000 - currentIterationEval + 1) / 2) : ((100000 - currentIterationEval) / 2));
			}
			else {
				ss << " score mate " << (((100000 + currentIterationEval) % 2) ? (-(100000 + currentIterationEval + 1) / 2) : (-(100000 + currentIterationEval) / 2));
			}
		}
		else {
			ss << " score cp " << currentIterationEval;
		}
		if (!newPv.empty()) {
			 ss << " pv"; displayPv = newPv; while (!displayPv.empty()) { ss << " " << misc.numToString(displayPv.top()); displayPv.pop(); };
		}
		ss << "\n";
		std::cout << ss.str();
		std::cout << std::flush;

		Stats::maxDepth = 0;
		Stats::partialnodes = 0;

		if (!Time::timed_out()) {
			if ((lastIterationEval + windowhigh) <= currentIterationEval) { windowhigh *= 3; }
			else if ((lastIterationEval - windowlow) >= currentIterationEval) { windowlow *= 3; }			
			else if (abs(currentIterationEval) > 90000) { lastIterationEval = currentIterationEval; lastPv = newPv; if (!newPv.empty()) { Bestmove = newPv.top(); } break; }
			else {
				Bestmove = newPv.top();

				depth--;
				nextDepth++;

				windowhigh = 50;
				windowlow = 50;

				lastIterationEval = currentIterationEval;
				lastPv = newPv;
			}
		}
		else {
			break;
		}

	}
	std::stringstream ss;

	ss << "info depth " << nextDepth << " nodes " << Stats::overallnodes;
	if (abs(lastIterationEval) > 90000) {
		if (lastIterationEval > 0) {
			ss << " score mate " << (((100000 - lastIterationEval) % 2) ? ((100000 - lastIterationEval + 1) / 2) : ((100000 - lastIterationEval) / 2));
		}
		else {
			ss << " score mate " << (((100000 + lastIterationEval) % 2) ? (-(100000 + lastIterationEval + 1) / 2) : (-(100000 + lastIterationEval) / 2));
		}
	}
	else {
		ss << " score cp " << lastIterationEval;
	}
	if (!newPv.empty()) {
		ss << " pv"; displayPv = newPv; while (!displayPv.empty()) { ss << " " << misc.numToString(displayPv.top()); displayPv.pop(); };
	}
	ss << " time " << Time::getMsec();
	ss << "\n";
	std::cout << ss.str();
	
	std::cout << "bestmove " << misc.numToString(Bestmove) << "\n";
}


void SearchInstance::sort(std::stack<Move>* &pv) {
	uint8_t sortIndex = 0;
	uint8_t reverseSortIndex = movenum - 1;

	if (pv != nullptr) {
		Move pvMove = (*pv).top();
		(*pv).pop();
		if ((*pv).empty()) { pv = nullptr; }

		sortedMovelist[0] = pvMove;
		sortIndex++;
	}
	for (int i = 0; i < movenum; ++i) {
		if (movelist[i] & flagmask) {
			sortedMovelist[sortIndex] = movelist[i];
			sortIndex++;
		}
		else {
			sortedMovelist[reverseSortIndex] = movelist[i];
			reverseSortIndex--;
		}
	} 
}


int SearchInstance::see(int exchangeIndex, Move m, int depth, int alpha, int beta, std::stack<Move>*& pv, std::stack<Move>* lowerPv) {

	this->gs.makemove(m);

	Movegenerator mg(&gs, movelist, &movenum);
	mg.generate();

	if (*movelist == checkmate) { return (-100000 + Stats::fullDepth - depth); }
	else if (*movelist == stalemate) { return 0; }

	sort(pv);

	int value = 0;
	Move index = 0;

	for (int i = 0; i < movenum; ++i){

		if (Time::timed_out()) { return 0; }

		std::stack<Move> higherPv;

		index = sortedMovelist[i] << 6;
		index >>= 10;

		if (index == exchangeIndex) {
			SearchInstance SI(&gs);
			value = -SI.see(exchangeIndex, sortedMovelist[i], depth - 1, -beta, -alpha, pv, &higherPv);
		}
		else {
			Stats::overallnodes++; Stats::partialnodes++;
			Stats::maxDepth = std::max(Stats::maxDepth, Stats::fullDepth - depth);
			Eval* eval = new Eval(&this->gs);
			int val = eval->get();
			delete eval;
			value = ((gs.white) ? 1 : -1) * val;
		}

		if (value >= beta) { return beta; }
		if (value > alpha) {
			alpha = value;
			higherPv.push(sortedMovelist[i]);
			*lowerPv = higherPv;
		}

	}

	return alpha;
}

int SearchInstance::bestmove(Move m, int depth, int alpha, int beta, std::stack<Move>* &pv, std::stack<Move>* lowerPv) {

	this->gs.makemove(m);

	if (stopFlag) { return 0; }

	if (!depth) {
		Stats::overallnodes++;
		Stats::partialnodes++;
		Eval* eval = new Eval(&this->gs);
		int val = eval->get();
		delete eval;
		return ((gs.white) ? 1 : -1) * val;
	}

	Movegenerator mg(&gs, movelist, &movenum);
	mg.generate();

	if (*movelist == checkmate) { return (-100000 + Stats::fullDepth - depth); }
	else if (*movelist == stalemate) { return 0; }
	
	sort(pv);

	int value = 0;
	Move captureIndex = 0;

	for (int i = 0; i < movenum; ++i){

		if (Time::timed_out()) { return 0; }

		std::stack<Move> higherPv;

		if (depth == 1 && (sortedMovelist[i] & flagmask) == capture) {
			captureIndex = sortedMovelist[i] << 6;
			captureIndex >>= 10;

			SearchInstance SI(&gs);
			value = -SI.see(captureIndex, sortedMovelist[i], depth - 1, -beta, -alpha, pv, &higherPv);
			if (value >= beta) { return beta; }
			if (value > alpha) {
				alpha = value;
				higherPv.push(sortedMovelist[i]);
				*lowerPv = higherPv;
			}
		}
		else {
			SearchInstance SI(&gs);
			value = -SI.bestmove(sortedMovelist[i], depth - 1, -beta, -alpha, pv, &higherPv);
			if (value >= beta) { return beta; }
			if (value > alpha) {
				alpha = value;
				higherPv.push(sortedMovelist[i]);
				*lowerPv = higherPv;
			}
		}

	}

	return alpha;
}

void SearchInstance::perft(Move m, int depth) {

	if (!depth) { Stats::partialnodes++; Stats::overallnodes++; return; }

	this->gs.makemove(m);

	Movegenerator mg(&gs, movelist, &movenum);
	mg.generate();

	if (*movelist == checkmate || *movelist == stalemate) { return; }

	for (int i = 0; i < movenum; ++i) {

		SearchInstance SI(&gs);
		SI.perft(movelist[i], depth - 1);

	}

}


void SearchInstance::newsearch(SType mode, int depth) {

	if (mode == PERFT){

		Movegenerator mg(&gs, movelist, &movenum);
		mg.generate();
		
		if (!(*movelist == checkmate || *movelist == stalemate)) {

			for (int i = 0; i < movenum; ++i) {

				SearchInstance SI(&gs);
				SI.perft(movelist[i], depth - 1);

				Misc misc;
				std::string move = misc.numToString(movelist[i]);
				std::cout << move << ": " << Stats::partialnodes << "\n";
				Stats::partialnodes = 0;


			}

		}
		else { std::cout << ((*movelist == checkmate) ? "Checkmate!" : "Stalemate!") << "\n"; }

		std::cout << "Total nodes searched: " << Stats::overallnodes << "\n";

	}
	else if (mode == BESTMOVE) {

		Time::start_time(horribleTimeVar);
		initID(depth);

	}

}
