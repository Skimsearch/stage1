package ulog;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.concurrent.TimeUnit;

import org.openjdk.jmh.annotations.Benchmark;
import org.openjdk.jmh.annotations.BenchmarkMode;
import org.openjdk.jmh.annotations.Fork;
import org.openjdk.jmh.annotations.Level;
import org.openjdk.jmh.annotations.Measurement;
import org.openjdk.jmh.annotations.Mode;
import org.openjdk.jmh.annotations.OutputTimeUnit;
import org.openjdk.jmh.annotations.Scope;
import org.openjdk.jmh.annotations.Setup;
import org.openjdk.jmh.annotations.State;
import org.openjdk.jmh.annotations.Warmup;

@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MICROSECONDS)
@State(Scope.Benchmark)
@Warmup(iterations = 2, time = 1, timeUnit = TimeUnit.SECONDS)
@Measurement(iterations = 5, time = 1, timeUnit = TimeUnit.SECONDS)
@Fork(1)
public class DatalakeBenchmark {

    private String baseDir;
    private final String targetBookId = "1342";

    @Setup(Level.Trial)
    public void setup() {
        Path p = Paths.get("../../datalake");
        if (!Files.exists(p)) {
            p = Paths.get("datalake");
        }
        baseDir = p.toAbsolutePath().toString();
    }


    @Benchmark
    public boolean lookupTimeBased() {
        Path body = Paths.get(baseDir, "20260922", "11", targetBookId + ".body.txt");
        if (!Files.exists(body)) {
            body = Paths.get(baseDir, "time", "20260923", "13", targetBookId + ".body.txt");
        }
        return Files.exists(body);
    }


    @Benchmark
    public boolean lookupBookBased() {
        Path body = Paths.get(baseDir, "book", targetBookId, targetBookId + ".body.txt");
        return Files.exists(body);
    }


    @Benchmark
    public boolean lookupBatchBased() {
        Path body = Paths.get(baseDir, "batch", "1000-1999", targetBookId + ".body.txt");
        return Files.exists(body);
    }


    @Benchmark
    public void writeBatchBenchmark() throws IOException {
        Path tempDir = Paths.get(baseDir, "temp_bench");
        if (!Files.exists(tempDir)) {
            Files.createDirectories(tempDir);
        }
        Path testFile = tempDir.resolve("bench_test.txt");
        Files.writeString(testFile, "Contenido de prueba para benchmark de escritura");
        Files.deleteIfExists(testFile);
    }
}