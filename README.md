# Datalake & Indexing Benchmarking System

This project is a comprehensive Information Retrieval and Data Engineering system designed to build, manage, and benchmark different indexing strategies across a datalake. The project features implementations in three different programming languages: **C, Java, and Python**, allowing for cross-language performance comparisons.

## 🚀 Features

*   **Data Ingestion**: Tools for downloading and crawling data (`downloader.py`, `crawler.c`).
*   **Text Processing**: Text tokenization capabilities (`tokenizer.py`).
*   **Indexing Strategies**:
    *   Standard Inverted Index
    *   Hierarchical Inverted Index
    *   MongoDB-backed Index
*   **Benchmarking**: Automated scripts and programs to benchmark index build times and query resource usage across all supported languages.

## 📁 Project Structure

The repository is organized into the following main directories:

*   **`datamart/` & `datamart_c/`**: Stores the generated metadata (`metadata.db`), the raw JSON inverted indexes (`inverted_index.json`), and the hierarchical file-based index structure (organized alphabetically).
*   **`source/c/`**: High-performance C implementations.
    *   Core logic: `datalake.c`, `indexer.c`, `hierarchical_index.c`, `mongodb_index.c`.
    *   Benchmarks: `datalake_benchmark.c`, `inverted_index_benchmark.c`.
*   **`source/java/`**: Java implementations built with Maven (`pom.xml`).
    *   Contains builders and benchmarkers inside the `ulog` package (e.g., `IndexBuilder.java`, `DatalakeBenchmark.java`, `InvertedIndexBenchmark.java`).
*   **`source/python/`**: Python scripts for data manipulation, ingestion, and rapid prototyping.
    *   Includes `crawler.py`, `tokenizer.py`, `inverted_index.py`, and benchmarking scripts.

## 🛠️ Prerequisites

To run all components of this project, you will need:

*   **C Environment**: GCC or Clang compiler, Make.
*   **Java Environment**: JDK 11 or higher, Apache Maven.
*   **Python Environment**: Python 3.8+, `pip` for dependency management.
*   **Database**: A running instance of MongoDB (required for the `mongodb_index` modules).

## 🚦 Getting Started

### Python Implementation
Navigate to the Python source directory and run the desired scripts. (It is recommended to use a virtual environment).
```bash
cd source/python
# Example: Run the index benchmark
python benchmark_index.py
```

### Java Implementation
The Java project uses Maven for dependency management and building.
```bash
cd source/java
mvn clean install
# Execute the benchmarks using java -cp or your IDE
java -cp target/classes ulog.RunInvertedIndexBenchmark
```

### C Implementation
Navigate to the C source directory and compile the executables.
```bash
cd source/c
# Compile (example assuming standard GCC)
gcc -o datalake datalake.c
gcc -o benchmark datalake_benchmark.c
./benchmark
```
*(Note: Refer to any local Makefiles or build scripts if provided for exact compilation flags, especially regarding MongoDB C-driver linkages).*

## 📊 Benchmarking

The project is heavily focused on benchmarking. Each language folder contains specific benchmark scripts:
*   **Python**: `benchmark_datalake.py`, `benchmark_index.py`
*   **Java**: `DatalakeBenchmark.java`, `IndexBuildBenchmark.java`, `IndexResourceBenchmark.java`
*   **C**: `datalake_benchmark.c`, `inverted_index_benchmark.c`

These scripts will typically evaluate the time taken to build the indexes and the latency/resource consumption when querying the datamart.

## Quick test with sample data

A small sample dataset is included in `sample_data/book/` so the pipeline can be tested without downloading the books again.

The sample contains the same six books used in the benchmarks:
- 11
- 84
- 98
- 1342
- 1661
- 2701

First, prepare the sample dataset:

```bash
./prepare_sample.sh
```
#### Python

```bash
python3 source/python/benchmark_index.py
```

#### C

Compile
```bash
gcc -o source/c/inverted_index_benchmark \
source/c/indexer.c \
source/c/hierarchical_index.c \
source/c/mongodb_index.c \
source/c/inverted_index_benchmark.c \
$(pkg-config --cflags --libs mongoc2)
```
Run
```bash
./source/c/inverted_index_benchmark
```

#### Java
Go to the Java project directory:
```bash
cd source/java
```
Build the project
```bash
mvn clean package
```

Then open:
src/main/java/ulog/RunInvertedIndexBenchmark.java
and run it using Run Java from your IDE.

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.