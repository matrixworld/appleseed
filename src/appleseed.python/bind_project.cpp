
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2012-2013 Esteban Tovagliari, Jupiter Jazz Limited
// Copyright (c) 2014 Esteban Tovagliari, The appleseedhq Organization
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// Has to be first, to avoid redefinition warnings.
#include "bind_auto_release_ptr.h"

// appleseed.python headers.
#include "bind_typed_entity_containers.h"
#include "dict2dict.h"

// appleseed.renderer headers.
#include "renderer/api/frame.h"
#include "renderer/api/project.h"
#include "renderer/api/scene.h"

// appleseed.foundation headers.
#include "foundation/utility/autoreleaseptr.h"
#include "foundation/utility/searchpaths.h"

// Standard headers.
#include <cstddef>
#include <string>

namespace bpy = boost::python;
using namespace foundation;
using namespace renderer;

namespace detail
{
    auto_release_ptr<Project> create_project(const std::string& name)
    {
        return ProjectFactory::create(name.c_str());
    }

    bpy::object create_default_project()
    {
        auto_release_ptr<Project> project(DefaultProjectFactory::create());
        return bpy::object(project);
    }

    bpy::object create_cornell_box_project()
    {
        auto_release_ptr<Project> project(CornellBoxProjectFactory::create());
        return bpy::object(project);
    }

    bpy::list project_get_search_paths(const Project* project)
    {
        bpy::list paths;

        const SearchPaths& search_paths = project->search_paths();

        for (size_t i = 0; i < search_paths.size(); ++i)
            paths.append(search_paths[i]);

        return paths;
    }

    void project_set_search_paths(Project* project, const bpy::list& paths)
    {
        project->search_paths().clear();

        for (bpy::ssize_t i = 0, e = bpy::len(paths); i < e; ++i)
        {
            const bpy::extract<const char*> extractor(paths[i]);
            if (extractor.check())
                project->search_paths().push_back(extractor());
            else
            {
                PyErr_SetString(PyExc_TypeError, "Incompatible type. Only strings accepted.");
                bpy::throw_error_already_set();
            }
        }
    }

    ConfigurationContainer* project_get_configs(Project* project)
    {
        return &(project->configurations());
    }

    bool write_project_default_opts(
        const ProjectFileWriter*            writer,
        const Project*                      project,
        const char*                         filepath)
    {
        return ProjectFileWriter::write(*project, filepath);
    }

    bool write_project_with_opts(
        const ProjectFileWriter*            writer,
        const Project*                      project,
        const char*                         filepath,
        const ProjectFileWriter::Options    opts)
    {
        return ProjectFileWriter::write(*project, filepath, opts);
    }

    auto_release_ptr<Configuration> create_config(const std::string& name)
    {
        return ConfigurationFactory::create(name.c_str());
    }

    auto_release_ptr<Configuration> create_config_with_params(
        const std::string&                  name,
        const bpy::dict&                    params)
    {
        return ConfigurationFactory::create(name.c_str(), bpy_dict_to_param_array(params));
    }

    bpy::object create_base_final_config()
    {
        auto_release_ptr<Configuration> config(BaseConfigurationFactory::create_base_final());
        return bpy::object(config);
    }

    bpy::object create_base_interactive_config()
    {
        auto_release_ptr<Configuration> config(BaseConfigurationFactory::create_base_interactive());
        return bpy::object(config);
    }

    bpy::dict config_get_inherited_parameters(const Configuration* config)
    {
        ParamArray params(config->get_inherited_parameters());
        return param_array_to_bpy_dict(params);
    }

    bpy::object project_file_reader_read(
        ProjectFileReader*                  reader,
        const char*                         project_filename,
        const char*                         schema_filename)
    {
        auto_release_ptr<Project> project(reader->read(project_filename, schema_filename));
        return bpy::object(project);
    }

    bpy::object project_file_reader_load_builtin(ProjectFileReader* reader, const char* project_name)
    {
        auto_release_ptr<Project> project(reader->load_builtin(project_name));
        return bpy::object(project);
    }
}

void bind_project()
{
    bpy::class_<Configuration, auto_release_ptr<Configuration>, bpy::bases<Entity>, boost::noncopyable>("Configuration", bpy::no_init)
        .def("create_base_final", detail::create_base_final_config).staticmethod("create_base_final")
        .def("create_base_interactive", detail::create_base_interactive_config).staticmethod("create_base_interactive")

        .def("__init__", bpy::make_constructor(detail::create_config))
        .def("__init__", bpy::make_constructor(detail::create_config_with_params))

        .def("set_base", &Configuration::set_base)
        .def("get_base", &Configuration::get_base, bpy::return_value_policy<bpy::reference_existing_object>())
        .def("get_inherited_parameters", detail::config_get_inherited_parameters);

    bind_typed_entity_map<Configuration>("ConfigurationContainer");

    bpy::class_<Project, auto_release_ptr<Project>, bpy::bases<Entity>, boost::noncopyable>("Project", bpy::no_init)
        .def("create_default", detail::create_default_project).staticmethod("create_default")
        .def("create_cornell_box", detail::create_cornell_box_project).staticmethod("create_cornell_box")

        .def("__init__", bpy::make_constructor(detail::create_project))

        .def("add_default_configurations", &Project::add_default_configurations)

        .def("has_path", &Project::has_path)
        .def("set_path", &Project::set_path)
        .def("get_path", &Project::get_path)

        .def("get_search_paths", detail::project_get_search_paths)
        .def("set_search_paths", detail::project_set_search_paths)

        .def("set_scene", &Project::set_scene)
        .def("get_scene", &Project::get_scene, bpy::return_value_policy<bpy::reference_existing_object>())

        .def("set_frame", &Project::set_frame)
        .def("get_frame", &Project::get_frame, bpy::return_value_policy<bpy::reference_existing_object>())

        .def("configurations", detail::project_get_configs, bpy::return_value_policy<bpy::reference_existing_object>())

        .def("create_aov_images", &Project::create_aov_images);

    bpy::class_<ProjectFileReader>("ProjectFileReader")
        .def("read", &detail::project_file_reader_read)
        .def("load_builtin", &detail::project_file_reader_load_builtin);

    bpy::enum_<ProjectFileWriter::Options>("ProjectFileWriterOptions")
        .value("Defaults", ProjectFileWriter::Defaults)
        .value("OmitHeaderComment", ProjectFileWriter::OmitHeaderComment)
        .value("OmitWritingMeshFiles", ProjectFileWriter::OmitWritingMeshFiles)
        .value("OmitBringingAssets", ProjectFileWriter::OmitBringingAssets)
        .value("OmitSearchPaths", ProjectFileWriter::OmitSearchPaths);

    bpy::class_<ProjectFileWriter>("ProjectFileWriter")
        // These methods are static, but for symmetry with
        // ProjectFileReader we're wrapping them non-static.
        .def("write", detail::write_project_default_opts)
        .def("write", detail::write_project_with_opts);
}
