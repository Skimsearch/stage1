package ulog;

import org.openjdk.jmh.annotations.*;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.*;
import java.util.concurrent.TimeUnit;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MILLISECONDS)
@State(Scope.Benchmark)
@Warmup(iterations = 2, time = 1, timeUnit = TimeUnit.SECONDS)
@Measurement(iterations = 5, time = 1, timeUnit = TimeUnit.SECONDS)
@Fork(1)
public class IndexBuildBenchmark {

    private static final Pattern TOKEN_PATTERN = Pattern.compile("\\b[a-zA-Z]+\\b");

    private static final List<Integer> BOOKS = Arrays.asList(
        1342,
        11,
        84,
        98,
        1661,
        2701
    );

    private List<String> tokenize(String text) {

        List<String> tokens = new ArrayList<>();

        Matcher matcher = TOKEN_PATTERN.matcher(text.toLowerCase(Locale.ROOT));

        while (matcher.find()) {
            tokens.add(matcher.group());
        }

        return tokens;
    }

    private Map<String, List<Integer>> buildInvertedIndex(List<Integer> bookIds) throws IOException {
        Map<String, List<Integer>> index = new HashMap<>();

        for (int bookId : bookIds) {

            Path bodyPath = Paths.get(
                "../../datalake/book/" + bookId + "/" + bookId + ".body.txt"
            );

            if (!Files.exists(bodyPath)) {
                bodyPath = Paths.get(
                    "datalake/book/" + bookId + "/" + bookId + ".body.txt"
                );
            }

            if (!Files.exists(bodyPath)) {
                continue;
            }

            String text = Files.readString(bodyPath);

            Set<String> uniqueTokens = new HashSet<>(tokenize(text));

            for (String token : uniqueTokens) {
                index
                    .computeIfAbsent(token, key -> new ArrayList<>())
                    .add(bookId);
            }
        }

        return index;
    }

    @Benchmark
    public Map<String, List<Integer>> buildIndexBenchmark() throws IOException {
        return buildInvertedIndex(BOOKS);
    }
}