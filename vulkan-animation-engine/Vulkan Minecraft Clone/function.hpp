// glm
#include <glm/glm.hpp>

// std
#include <vector>

namespace vae {
	class Function {
	public:
		Function();
		
		std::vector<glm::vec3>& getCurvePoints() { return curvePoints; };

	private:
		void generateCurvePoints();
		std::vector<glm::vec3> curvePoints;
	};
}