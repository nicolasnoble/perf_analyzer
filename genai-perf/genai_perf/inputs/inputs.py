# Copyright 2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import json
import random
from copy import deepcopy
from pathlib import Path
from typing import Any, Dict, List, Tuple

from genai_perf.exceptions import GenAIPerfException
from genai_perf.inputs.input_constants import (
    DEFAULT_INPUT_DATA_JSON,
    MINIMUM_LENGTH,
    MINIMUM_STARTING_INDEX,
    OutputFormat,
    PromptSource,
)
from genai_perf.inputs.input_retriever_factory import InputRetrieverFactory
from genai_perf.inputs.inputs_config import InputsConfig
from genai_perf.inputs.output_format_converter_factory import (
    OutputFormatConverterFactory,
)


class Inputs:
    """
    A class that generates the request input payloads for the target endpoint.
    """

    def __init__(self, config: InputsConfig):
        self.config = config
        if self.config.extra_inputs is None:
            self.config.extra_inputs = {}

        random.seed(self.config.random_seed)

    def create_inputs(self) -> Dict:
        """
        Given an input type, input format, and output type. Output a string of LLM Inputs
        (in a JSON dictionary) to a file.
        """
        self._check_for_valid_args()
        input_retriever = InputRetrieverFactory(self.config)
        generic_dataset_json = input_retriever.get_input_data()

        json_in_pa_format = self._convert_generic_json_to_output_format(
            generic_dataset_json,
        )
        self._write_json_to_file(json_in_pa_format)
        return json_in_pa_format

    def _check_for_valid_args(self) -> None:
        self._check_for_dataset_name_if_input_type_is_url()
        self._check_for_tokenzier_if_input_type_is_synthetic()
        self._check_for_valid_starting_index()
        self._check_for_valid_length()

    def _verify_file(self) -> None:
        if not self.config.input_filename.exists():
            raise FileNotFoundError(
                f"The file '{self.config.input_filename}' does not exist."
            )

    def _convert_generic_json_to_output_format(self, generic_dataset) -> Dict:
        converter = OutputFormatConverterFactory.create(self.config.output_format)
        return converter.convert(generic_dataset, self.config)

    def _write_json_to_file(self, json_in_pa_format: Dict) -> None:
        filename = self.config.output_dir / DEFAULT_INPUT_DATA_JSON
        with open(str(filename), "w") as f:
            f.write(json.dumps(json_in_pa_format, indent=2))

    def _check_for_dataset_name_if_input_type_is_url(self) -> None:
        if (
            self.config.input_type == PromptSource.DATASET
            and not self.config.dataset_name
        ):
            raise GenAIPerfException(
                "Input type is dataset, but dataset_name is not specified."
            )

    def _check_for_tokenzier_if_input_type_is_synthetic(self) -> None:
        if (
            self.config.input_type == PromptSource.SYNTHETIC
            and not self.config.tokenizer
        ):
            raise GenAIPerfException(
                "Input type is SYNTHETIC, but a tokenizer was not specified."
            )

    def _check_for_valid_starting_index(self) -> None:
        if not isinstance(self.config.starting_index, int):
            raise GenAIPerfException(
                f"starting_index: {self.config.starting_index} must be an integer."
            )

        if self.config.starting_index < MINIMUM_STARTING_INDEX:
            raise GenAIPerfException(
                f"starting_index: {self.config.starting_index} must be larger than {MINIMUM_STARTING_INDEX}."
            )

    def _check_for_valid_length(self) -> None:
        if not isinstance(self.config.length, int):
            raise GenAIPerfException(
                f"length: {self.config.length} must be an integer."
            )

        if self.config.length < MINIMUM_LENGTH:
            raise GenAIPerfException(
                f"starting_index: {self.config.length} must be larger than {MINIMUM_LENGTH}."
            )
