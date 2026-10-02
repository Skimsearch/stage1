package ulog;

import org.openjdk.jmh.annotations.*;

import java.io.IOException;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MILLISECONDS)
@State(Scope.Benchmark)
@Warmup(iterations = 2, time = 1, timeUnit = TimeUnit.SECONDS)
@Measurement(iterations = 5, time = 1, timeUnit = TimeUnit.SECONDS)
@Fork(1)
public class IndexBuildBenchmark {

    @Benchmark
    public Map<String, List<Integer>> buildIndexBenchmark() throws IOException {
        return IndexBuilder.buildInvertedIndex(IndexBuilder.ALL_BOOKS);
    }
}