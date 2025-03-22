import os
import framework.generator as gen
import framework.scheme as sc


class BasicCPPGenerator(gen.GeneratorBase):
    def pre_generate(self):
        script_dir = os.path.dirname(os.path.realpath(__file__).replace("\\", "/"))
        header_top_template = self.owner.load_template(os.path.join(script_dir, "header_top.mako"))
        source_top_template = self.owner.load_template(os.path.join(script_dir, "source_top.mako"))

        # test generate body
        for record in self.owner.database.get_records():
            if record.generated_body_content and not record.has_generate_body_flag:
                self.owner.logger.error(f"Record {record.name} has generated body content but lost SKR_GENERATE_BODY()", [record.make_log_stack()])

        # gen header
        for header_db in self.owner.database.main_module.header_dbs:
            self.owner.append_content(
                header_db.relative_target_header_path,
                header_top_template.render(header_db=header_db)
            )

        # gen source
        self.owner.append_content(
            "generated.cpp",
            source_top_template.render(module_db=self.owner.database.main_module)
        )

    def post_generate(self):
        script_dir = os.path.dirname(os.path.realpath(__file__).replace("\\", "/"))
        header_bottom_template = self.owner.load_template(os.path.join(script_dir, "header_bottom.mako"))
        source_bottom_template = self.owner.load_template(os.path.join(script_dir, "source_bottom.mako"))

        # gen header
        for header_db in self.owner.database.main_module.header_dbs:
            self.owner.append_content(
                header_db.relative_target_header_path,
                header_bottom_template.render(header_db=header_db)
            )

        # gen source
        self.owner.append_content(
            "generated.cpp",
            source_bottom_template.render(module_db=self.owner.database.main_module)
        )


def load_generators(generate_manager: gen.GenerateManager):
    generate_manager.add_generator("basic", BasicCPPGenerator())
