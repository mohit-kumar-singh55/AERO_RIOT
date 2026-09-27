#pragma once

#include <utility>

class Collider;

struct CollisionPair {
	CollisionPair(Collider* first, Collider* second) {
		// if second < first
		if (std::less<Collider*>{}(second, first))
			std::swap(first, second);

		a = first;
		b = second;
	}

	bool operator==(const CollisionPair& other) const noexcept {
		return a == other.a && b == other.b;
	}

	Collider* a;	// smaller
	Collider* b;	// higher
};

struct CollisionPairHash {
	std::size_t operator()(const CollisionPair& pair) const noexcept {
		// calculate hash
		auto hashA = std::hash<Collider*>{}(pair.a);
		auto hashB = std::hash<Collider*>{}(pair.b);

		return hashA ^ (hashB << 1);
	}
};