package ulog;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;
import org.openjdk.jmh.annotations.*;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.lang.reflect.Type;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.*;
import java.util.concurrent.TimeUnit;

@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MILLISECONDS)
@State(Scope.Benchmark)
@Warmup(iterations = 2, time = 1, timeUnit = TimeUnit.SECONDS)
@Measurement(iterations = 5, time = 1, timeUnit = TimeUnit.SECONDS)
@Fork(1)
public class InvertedIndexBenchmark {

    private String jsonPath;
    private String hierDir;
    private Map<String, List<Integer>> memoryJsonIndex;
    private List<String> queryWords;

    @Setup(Level.Trial)
    public void setup() throws IOException {
        queryWords = Arrays.asList("abandon", "abandoned", "abound", "about", "absence");

        Path pJson = Paths.get("../../datamarts/inverted_index/inverted_index.json");
        if (!Files.exists(pJson)) {
            pJson = Paths.get("datamarts/inverted_index/inverted_index.json");
        }
        jsonPath = pJson.toAbsolutePath().toString();

        Path pHier = Paths.get("../../datamart/inverted_index_hierarchical");
        if (!Files.exists(pHier)) {
            pHier = Paths.get("datamart/inverted_index_hierarchical");
        }
        hierDir = pHier.toAbsolutePath().toString();

        Gson gson = new Gson();
        Type type = new TypeToken<Map<String, List<Integer>>>(){}.getType();
        try (BufferedReader reader = new BufferedReader(new FileReader(jsonPath))) {
            memoryJsonIndex = gson.fromJson(reader, type);
        }
    }


    @Benchmark
    public List<Integer> queryMonolithicJson() {
        List<Integer> allPostings = new ArrayList<>();
        for (String word : queryWords) {
            List<Integer> postings = memoryJsonIndex.get(word);
            if (postings != null) {
                allPostings.addAll(postings);
            }
        }
        return allPostings;
    }


    @Benchmark
    public List<Integer> queryHierarchicalTxt() throws IOException {
        List<Integer> allPostings = new ArrayList<>();
        for (String word : queryWords) {
            String firstLetter = word.substring(0, 1).toUpperCase();
            Path termFile = Paths.get(hierDir, firstLetter, word + ".txt");
            if (Files.exists(termFile)) {
                List<String> lines = Files.readAllLines(termFile);
                for (String line : lines) {
                    for (String token : line.trim().split("\\s+")) {
                        if (!token.isEmpty()) {
                            try {
                                allPostings.add(Integer.parseInt(token));
                            } catch (NumberFormatException ignored) {}
                        }
                    }
                }
            }
        }
        return allPostings;
    }
}