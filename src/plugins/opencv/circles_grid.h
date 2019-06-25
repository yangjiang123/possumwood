#pragma once

#include <memory>
#include <iostream>

#include <opencv2/opencv.hpp>

#include <actions/traits.h>

namespace possumwood { namespace opencv {

class CirclesGrid {
	public:
		enum Type {
			kSymmetricGrid,
			kAsymmetricGrid,
		};

		CirclesGrid(const cv::Mat& data = cv::Mat(), const cv::Size& size = cv::Size(0,0), bool wasFound = false, const Type& type = kSymmetricGrid);

		const cv::Mat& operator*() const;
		const cv::Size& size() const;
		Type type() const;
		bool wasFound() const;

		bool operator == (const CirclesGrid& f) const;
		bool operator != (const CirclesGrid& f) const;

	private:
		std::shared_ptr<const cv::Mat> m_features; // 2D or 3D list of points
		cv::Size m_size;
		bool m_wasFound;
		Type m_type;
};

std::ostream& operator << (std::ostream& out, const CirclesGrid& f);

}

template<>
struct Traits<opencv::CirclesGrid> {
	static constexpr std::array<float, 3> colour() {
		return std::array<float, 3>{{0.3, 0.3, 0}};
	}
};

}
