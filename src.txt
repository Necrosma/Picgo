fn main() -> void {
    let count: int;
    count = getint();
    while count > 0 {
        let needle: int;
        let low: int = 0;
        let high: int = 1024;
        let mid: int = -3;
        needle = getint();
        while low != high {
            let mid: int = (low + high + 1) / 2;
            putint(mid);
            putln();
            if mid > needle {
                high = mid;
            } else if mid == needle {
                low = mid;
                high = mid;
            } else {
                low = mid;
            }
        }
        putln();
        count = count - 1;
    }
}
