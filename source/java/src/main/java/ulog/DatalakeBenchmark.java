package ulog;

import java.io.IOException;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.*;
import java.util.stream.Stream;


public class DatalakeBenchmark {

    private static final int[] BOOKS = {1342, 11, 84, 98, 1661};
    private static final String[] STRATEGIES = {"time", "book", "batch"};
    private static final String START_MARKER = "*** START OF THE PROJECT GUTENBERG EBOOK";
    private static final String END_MARKER = "*** END OF THE PROJECT GUTENBERG EBOOK";

    private static final Path DATALAKE_ROOT = resolveDatalakeRoot();

    private static final HttpClient HTTP_CLIENT = HttpClient.newHttpClient();

    private static Path resolveDatalakeRoot() {
        Path p = Paths.get("../../datalake");
        if (!Files.exists(p)) {
            p = Paths.get("datalake");
        }
        return p.toAbsolutePath().normalize();
    }

    // Path resolution (mirrors get_output_path / resolve_time_path)
    private static Path getOutputPath(int bookId, String strategy, LocalDateTime when) {
        switch (strategy) {
            case "time": {
                LocalDateTime ts = (when != null) ? when : LocalDateTime.now();
                String day = ts.format(DateTimeFormatter.ofPattern("yyyyMMdd"));
                String hour = ts.format(DateTimeFormatter.ofPattern("HH"));
                return DATALAKE_ROOT.resolve("time").resolve(day).resolve(hour);
            }
            case "book":
                return DATALAKE_ROOT.resolve("book").resolve(String.valueOf(bookId));
            case "batch": {
                int batchStart = (bookId / 1000) * 1000;
                int batchEnd = batchStart + 999;
                return DATALAKE_ROOT.resolve("batch").resolve(batchStart + "-" + batchEnd);
            }
            default:
                throw new IllegalArgumentException("Unknown strategy: " + strategy);
        }
    }

    private static Path resolveTimePath(int bookId) throws IOException {
        Path timeRoot = DATALAKE_ROOT.resolve("time");
        if (!Files.exists(timeRoot)) {
            return null;
        }
        String headerName = bookId + ".header.txt";
        String bodyName = bookId + ".body.txt";

        try (Stream<Path> walk = Files.walk(timeRoot)) {
            Optional<Path> match = walk
                    .filter(p -> p.getFileName().toString().equals(headerName)
                            || p.getFileName().toString().equals(bodyName))
                    .findFirst();
            return match.map(Path::getParent).orElse(null);
        }
    }

    private enum BookState { COMPLETE, INCOMPLETE, MISSING }

    private static BookState checkBookState(int bookId, String strategy) throws IOException {
        Path basePath;
        if (strategy.equals("time")) {
            basePath = resolveTimePath(bookId);
            if (basePath == null) {
                return BookState.MISSING;
            }
        } else {
            basePath = getOutputPath(bookId, strategy, null);
        }

        boolean headerExists = Files.exists(basePath.resolve(bookId + ".header.txt"));
        boolean bodyExists = Files.exists(basePath.resolve(bookId + ".body.txt"));

        if (headerExists && bodyExists) return BookState.COMPLETE;
        if (headerExists || bodyExists) return BookState.INCOMPLETE;
        return BookState.MISSING;
    }

    private static boolean findBook(int bookId, String strategy) throws IOException {
        return checkBookState(bookId, strategy) == BookState.COMPLETE;
    }

    // Network-only / disk-only split (mirrors fetch_book / save_book)
    private record BookText(String header, String body) {}

    private static BookText fetchBook(int bookId) throws IOException, InterruptedException {
        String url = "https://www.gutenberg.org/cache/epub/" + bookId + "/pg" + bookId + ".txt";
        HttpRequest request = HttpRequest.newBuilder(URI.create(url))
                .timeout(java.time.Duration.ofSeconds(20))
                .GET()
                .build();

        HttpResponse<String> response = HTTP_CLIENT.send(request, HttpResponse.BodyHandlers.ofString());
        // HttpClient does not throw on 4xx/5xx by itself, unlike Python's
        // requests.raise_for_status(); check explicitly for parity.
        if (response.statusCode() != 200) {
            throw new IOException("Request failed for book " + bookId + " with status " + response.statusCode());
        }
        String text = response.body();

        int startIdx = text.indexOf(START_MARKER);
        int endIdx = text.indexOf(END_MARKER);
        if (startIdx == -1 || endIdx == -1) {
            System.out.println("Gutenberg markers not found");
            return null;
        }

        String header = text.substring(0, startIdx).strip();
        String body = text.substring(startIdx + START_MARKER.length(), endIdx).strip();
        return new BookText(header, body);
    }

    private static void saveBook(int bookId, String header, String body, String strategy, LocalDateTime when) throws IOException {
        Path outputPath = getOutputPath(bookId, strategy, when);
        Files.createDirectories(outputPath);

        Files.writeString(outputPath.resolve(bookId + ".header.txt"), header, StandardCharsets.UTF_8);
        Files.writeString(outputPath.resolve(bookId + ".body.txt"), body, StandardCharsets.UTF_8);
    }

    // 1. Download / write throughput
    private static void benchmarkDownloadThroughput() throws IOException, InterruptedException {
        System.out.println("\nDOWNLOAD / WRITE THROUGHPUT BENCHMARK");

        Map<Integer, BookText> fetched = new LinkedHashMap<>();

        long start = System.nanoTime();
        for (int bookId : BOOKS) {
            BookText result = fetchBook(bookId);
            if (result == null) {
                System.out.println("Book " + bookId + " could not be parsed (markers not found)");
                continue;
            }
            fetched.put(bookId, result);
        }
        double networkTime = (System.nanoTime() - start) / 1e9;

        System.out.println("\nnetwork (shared across strategies):");
        System.out.printf("  Total fetch time: %.2f s%n", networkTime);
        System.out.println("  Books fetched: " + fetched.size());
        System.out.printf("  Average per book: %.2f s%n", networkTime / fetched.size());

        LocalDateTime when = LocalDateTime.now();

        for (String strategy : STRATEGIES) {
            start = System.nanoTime();
            for (Map.Entry<Integer, BookText> entry : fetched.entrySet()) {
                saveBook(entry.getKey(), entry.getValue().header(), entry.getValue().body(), strategy, when);
            }
            double diskTime = (System.nanoTime() - start) / 1e9;

            System.out.println("\n" + strategy + " (disk write only):");
            System.out.printf("  Total time: %.4f s%n", diskTime);
            System.out.println("  Books: " + fetched.size());
            System.out.printf("  Average per book: %.6f s%n", diskTime / fetched.size());
        }
    }

    // 2. Lookup cost
    private static void benchmarkLookup(int repetitions) throws IOException {
        System.out.println("\nLOOKUP BENCHMARK");

        Map<String, Integer> testIds = new LinkedHashMap<>();
        testIds.put("existing", BOOKS[0]);
        testIds.put("missing", 999999);

        for (String strategy : STRATEGIES) {
            System.out.println("\n" + strategy + ":");
            for (Map.Entry<String, Integer> entry : testIds.entrySet()) {
                boolean found = false;
                long start = System.nanoTime();
                for (int i = 0; i < repetitions; i++) {
                    found = findBook(entry.getValue(), strategy);
                }
                double totalTime = (System.nanoTime() - start) / 1e9;

                System.out.printf("  [%s] repetitions: %d, total: %.6fs, avg: %.8fs, found: %b%n",
                        entry.getKey(), repetitions, totalTime, totalTime / repetitions, found);
            }
        }
    }

    // 3. Storage overhead
    private record StorageStats(long files, long folders, long totalSize) {}

    private static StorageStats storageOverhead(String strategy) throws IOException {
        Path root = DATALAKE_ROOT.resolve(strategy);
        long[] files = {0};
        long[] folders = {0};
        long[] totalSize = {0};

        if (!Files.exists(root)) {
            return new StorageStats(0, 0, 0);
        }

        try (Stream<Path> walk = Files.walk(root)) {
            walk.forEach(p -> {
                if (Files.isRegularFile(p)) {
                    files[0]++;
                    try {
                        totalSize[0] += Files.size(p);
                    } catch (IOException ignored) {}
                } else if (Files.isDirectory(p) && !p.equals(root)) {
                    folders[0]++;
                }
            });
        }
        return new StorageStats(files[0], folders[0], totalSize[0]);
    }

    private static void benchmarkStorageOverhead() throws IOException {
        System.out.println("\nSTORAGE OVERHEAD BENCHMARK");

        for (String strategy : STRATEGIES) {
            StorageStats stats = storageOverhead(strategy);
            System.out.println("\n" + strategy + ":");
            System.out.println("  Files: " + stats.files());
            System.out.println("  Folders: " + stats.folders());
            System.out.printf("  Total size: %.2f KB%n", stats.totalSize() / 1024.0);
        }
    }

    // 4. Incremental processing
    private static Set<Integer> getPendingBooks(int[] booksList, String strategy, Set<Integer> indexedBooks) throws IOException {
        Set<Integer> available = new TreeSet<>();
        for (int bookId : booksList) {
            if (findBook(bookId, strategy)) {
                available.add(bookId);
            }
        }
        available.removeAll(indexedBooks);
        return available;
    }

    private static void benchmarkIncrementalProcessing() throws IOException {
        System.out.println("\nINCREMENTAL PROCESSING BENCHMARK");

        Set<Integer> mockIndexedBooks = new HashSet<>(Arrays.asList(11, 84));

        for (String strategy : STRATEGIES) {
            long start = System.nanoTime();
            Set<Integer> pending = getPendingBooks(BOOKS, strategy, mockIndexedBooks);
            double elapsed = (System.nanoTime() - start) / 1e9;

            System.out.println("\n" + strategy + ":");
            System.out.printf("  Detection time: %.6f s%n", elapsed);
            System.out.println("  Books ready to index: " + pending);
        }
    }

    // 5. Recovery behavior
    private static Path simulateInterruption(int bookId, String strategy) throws IOException {
        Path basePath = strategy.equals("time") ? resolveTimePath(bookId) : getOutputPath(bookId, strategy, null);
        if (basePath == null) {
            return null;
        }

        Path bodyPath = basePath.resolve(bookId + ".body.txt");
        if (!Files.exists(bodyPath)) {
            return null;
        }

        Path backupPath = basePath.resolve(bookId + ".body.txt.bak");
        Files.move(bodyPath, backupPath);
        return backupPath;
    }

    private static void restoreFromInterruption(Path backupPath) throws IOException {
        if (backupPath == null || !Files.exists(backupPath)) {
            return;
        }
        String name = backupPath.getFileName().toString();
        Path originalPath = backupPath.getParent().resolve(name.substring(0, name.length() - ".bak".length()));
        Files.move(backupPath, originalPath);
    }

    private static void benchmarkRecoveryBehavior() throws IOException {
        System.out.println("\nRECOVERY BEHAVIOR BENCHMARK");

        int corruptBookId = BOOKS[0];

        for (String strategy : STRATEGIES) {
            Path backupPath = simulateInterruption(corruptBookId, strategy);

            long start = System.nanoTime();

            List<Integer> complete = new ArrayList<>();
            List<Integer> incomplete = new ArrayList<>();
            List<Integer> missing = new ArrayList<>();

            for (int bookId : BOOKS) {
                BookState state = checkBookState(bookId, strategy);
                switch (state) {
                    case COMPLETE -> complete.add(bookId);
                    case INCOMPLETE -> incomplete.add(bookId);
                    case MISSING -> missing.add(bookId);
                }
            }

            double elapsed = (System.nanoTime() - start) / 1e9;

            System.out.println("\n" + strategy + ":");
            System.out.printf("  Recovery detection time: %.6f s%n", elapsed);
            System.out.println("  Complete (skip, avoid duplicate work): " + complete);
            System.out.println("  Incomplete (resume without duplicating): " + incomplete);
            System.out.println("  Missing (download from scratch): " + missing);

            restoreFromInterruption(backupPath);
        }
    }

    public static void main(String[] args) throws Exception {
        benchmarkDownloadThroughput();
        benchmarkLookup(1000);
        benchmarkStorageOverhead();
        benchmarkIncrementalProcessing();
        benchmarkRecoveryBehavior();
    }
}