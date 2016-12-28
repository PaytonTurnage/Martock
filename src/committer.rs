use body;
use commit;
use world;

pub enum CL {}

impl CL {
    pub fn commits(&self) -> Vec<commit::Commit> {
        Vec::new()
    }
}

pub trait Committer {
    fn cl(&self, &world::World, &[&body::Body]) -> Option<CL>;
}
