#include <iostream>
#include <fstream>

#include <GL/glew.h>
#include <GL/glut.h>

#include <boost/format.hpp>

#include <possumwood_sdk/app.h>
#include <possumwood_sdk/viewport_state.h>

#include "common.h"
#include "options.h"
#include "render_context.h"
#include "stack.h"
#include "expression.h"


// The CLI unfortunately has to be designed around GLUT - it is not possible in current
// version of GLUT to re-enter the event loop, and OSmesa does not support GL>2.0 on
// Debian.

// The RenderContext object provides an "execution engine" wired around GLUT to evaluate
// each step of execution inside the OpenGL loop. It initialises the OpenGL on the first
// --render parameter, and destroys it once the evaluation of all parameters finishes.


// global viewport state
possumwood::ViewportState viewport;
// rendering options
std::unique_ptr<RenderContext> ctx;

// global application instance
std::unique_ptr<possumwood::App> papp;

// frame step
std::size_t frame_step = 0;

// expression expansion
int currentFrame() {
	return std::round(possumwood::App::instance().time() * possumwood::App::instance().sceneConfig()["fps"].as<float>());
};

float currentTime() {
	return possumwood::App::instance().time();
};

const ExpressionExpansion expr({
	{"T", []() { return (boost::format("%.2f") % currentTime()).str(); }},
	{"F", []() { return (boost::format("%d") % currentFrame()).str(); }},
	{"2F", []() { return (boost::format("%02d") % currentFrame()).str(); }},
	{"3F", []() { return (boost::format("%03d") % currentFrame()).str(); }},
	{"4F", []() { return (boost::format("%04d") % currentFrame()).str(); }}
});

void printHelp() {
	std::cout << "Parameters:" << std::endl;
	std::cout << "  --scene <filename> - Loads a .psw scene file." << std::endl;
	std::cout << "  --render <filename> - renders a frame to a file. Only PPM files supported at the moment." << std::endl;
	std::cout << "  --window <width> <height> - defines the render window size in pixels" << std::endl;
	std::cout << "  --cam_pos <x> <y> <z> - defines camera position in world space" << std::endl;
	std::cout << "  --cam_target <x> <y> <z> - defines camera target (default 0,0,0)" << std::endl;
	std::cout << "  --frame_step <step> - render multiple frames" << std::endl;
	std::cout << std::endl;
	std::cout << "The render filename parameter can contain the following 'variables':" << std::endl;
	std::cout << "  $T - time, with two decimal points" << std::endl;
	std::cout << "  $F - frame, as an integer value" << std::endl;
	std::cout << "  $2F - frame, as an integer value, padded with 0s to the width of 2" << std::endl;
	std::cout << "  $3F - frame, as an integer value, padded with 0s to the width of 3" << std::endl;
	std::cout << "  $4F - frame, as an integer value, padded with 0s to the width of 4" << std::endl;
	std::cout << std::endl;
}

void loadScene(const Options::Item& option) {
	if(option.parameters.size() != 1)
		throw std::runtime_error("--scene option allows only exactly one filename");

	std::cout << "Loading " << option.parameters[0] << "... " << std::flush;
	papp->loadFile(boost::filesystem::path(option.parameters[0]));
	std::cout << "done" << std::endl;
}

std::vector<Action> render(const Options::Item& option) {
	if(option.parameters.size() != 1)
		throw std::runtime_error("--render option allows only exactly one filename");

	std::vector<Action> result;

	std::size_t end_param = 0;
	if(frame_step > 0) {
		const possumwood::Config& cfg = possumwood::App::instance().sceneConfig();
		end_param = std::size_t(round((cfg["end_time"].as<float>() - cfg["start_time"].as<float>()) * cfg["fps"].as<float>())) / frame_step;
	}

	for(std::size_t param = 0; param <= end_param; ++param) {
		std::function<void(std::vector<GLubyte>&)> callback = [option, param](std::vector<GLubyte>& buffer) {
			const possumwood::Config& cfg = possumwood::App::instance().sceneConfig();
			const float t = (float)(param * frame_step) / cfg["fps"].as<float>() + cfg["start_time"].as<float>();
			possumwood::App::instance().setTime(t);

			const std::string filename = expr.expand(option.parameters[0]);
			std::cout << "Rendering " << filename << "... " << std::flush;

			std::ofstream file(filename.c_str(), std::ofstream::binary);

			file << "P6" << std::endl;
			file << viewport.width() << " " << viewport.height() << " 255" << std::endl;

			for(unsigned l=viewport.height(); l>0; --l)
				file.write((const char*)(&buffer[(l-1) * viewport.width()*3]), viewport.width()*3);

			std::cout << "done" << std::endl;
		};

		result.push_back(ctx->render(viewport, callback));
	}

	return result;
}

std::vector<Action> evaluateOption(const Options::const_iterator& current) {
	const Options::Item& option = *current;

	if(option.name == "--scene")
		loadScene(option);

	else if(option.name == "--render")
		return render(option);

	else if(option.name == "--help")
		printHelp();

	else if(option.name == "--frame_step") {
		if(option.parameters.size() != 1)
			throw std::runtime_error("--frame_step option allows only exactly one integer parameter");

		frame_step = atoi(option.parameters[0].c_str());
	}

	else if(option.name == "--cam_pos") {
		if(option.parameters.size() != 3)
			throw std::runtime_error("--cam_pos option allows only exactly three floating-point parameter");

		float x = atof(option.parameters[0].c_str());
		float y = atof(option.parameters[1].c_str());
		float z = atof(option.parameters[2].c_str());

		viewport.lookAt(Imath::V3f(x, y, z), viewport.target());
	}

	else if(option.name == "--cam_target") {
		if(option.parameters.size() != 3)
			throw std::runtime_error("--cam_pos option allows only exactly three floating-point parameter");

		float x = atof(option.parameters[0].c_str());
		float y = atof(option.parameters[1].c_str());
		float z = atof(option.parameters[2].c_str());

		viewport.lookAt(viewport.eyePosition(), Imath::V3f(x, y, z));
	}

	else if(option.name == "--window") {
		if(option.parameters.size() != 2)
			throw std::runtime_error("--window option allows only exactly two integer parameters");

		viewport.resize(
			atoi(option.parameters[0].c_str()),
			atoi(option.parameters[1].c_str()));
	}

	else
		throw std::runtime_error("Unknown command line option " + option.name);

	return std::vector<Action>();
}

std::vector<Action> evaluateOptions(const Options& options) {
	std::vector<Action> result;

	for(auto it = options.begin(); it != options.end(); ++it)
		result.push_back(Action(std::bind(evaluateOption, it)));

	return result;
}

int main(int argc, char* argv[]) {
	// create the possumwood application
	papp = std::unique_ptr<possumwood::App>(new possumwood::App());

	// load all plugins into an RAII container
	PluginsRAII plugins;

	// parse the program options
	Options options(argc, argv);

	// populate the initial action
	Stack s;
	s.add(Action([&]() {
		std::vector<Action> result;
		result.push_back(Action(std::bind(evaluateOptions, std::cref(options))));
		return result;
	}));

	// two types of loops - "non-GL" loop when no --render parameter passed
	//                    - "GL" loop based on GLUT when at least one --render parameter is present
	auto renderParamIt = std::find_if(options.begin(), options.end(),
		[](const Options::Item& item) {
			return item.name == "--render";
		});

	if(renderParamIt != options.end()) {
		// use GL-based loop
		ctx = std::unique_ptr<RenderContext>(new RenderContext(viewport));
		ctx->run(s);
	}

	else {
		// use a trivial while statement
		while(!s.isFinished())
			s.step();
	}

	return 0;
}
